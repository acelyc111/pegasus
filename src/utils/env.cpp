// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "env.h"

#include <fmt/core.h>
#include <rocksdb/convenience.h>
#include <rocksdb/env.h>
#include <rocksdb/env_encryption.h>
#include <rocksdb/slice.h>
#include <algorithm>
#include <memory>
#include <string>

#include "utils/filesystem.h"
#include "utils/flags.h"
#include "utils/fmt_logging.h"
#include "utils/utils.h"

DSN_DEFINE_bool(pegasus.server,
                encrypt_data_at_rest,
                true,
                "Whether sensitive files should be encrypted on the file system.");

DSN_DEFINE_string(pegasus.server,
                  server_key_for_testing,
                  "server_key_for_testing",
                  "The encrypted server key to use in the filesystem. NOTE: only for testing.");

DSN_DEFINE_string(pegasus.server,
                  encryption_method,
                  "AES128CTR",
                  "The encryption method to use in the filesystem. Now "
                  "supports AES128CTR, AES192CTR, AES256CTR and SM4CTR.");

namespace dsn {
namespace utils {

rocksdb::Env *NewEncryptedEnv()
{
    std::string provider_id =
        fmt::format("AES:{},{}", FLAGS_server_key_for_testing, FLAGS_encryption_method);
    std::shared_ptr<rocksdb::EncryptionProvider> provider;
    auto s = rocksdb::EncryptionProvider::CreateFromString(
        rocksdb::ConfigOptions(), provider_id, &provider);
    CHECK(s.ok(), "Failed to create encryption provider: {}", s.ToString());
    return NewEncryptedEnv(rocksdb::Env::Default(), provider);
}

rocksdb::Env *PegasusEnv(FileDataType type)
{
    if (FLAGS_encrypt_data_at_rest && type == FileDataType::kSensitive) {
        static rocksdb::Env *env = NewEncryptedEnv();
        return env;
    }

    static rocksdb::Env *env = rocksdb::Env::Default();
    return env;
}

rocksdb::Status do_copy_file(const std::string &src_fname,
                             dsn::utils::FileDataType src_type,
                             const std::string &dst_fname,
                             dsn::utils::FileDataType dst_type,
                             int64_t remain_size,
                             uint64_t *total_size)
{
    rocksdb::EnvOptions rd_env_options;
    std::unique_ptr<rocksdb::SequentialFile> sfile;
    auto s = dsn::utils::PegasusEnv(src_type)->NewSequentialFile(src_fname, &sfile, rd_env_options);
    LOG_AND_RETURN_NOT_RDB_OK(WARNING, s, "failed to open file {} for reading", src_fname);

    // Limit the size of the file to be copied.
    int64_t src_file_size;
    CHECK(dsn::utils::filesystem::file_size(src_fname, src_type, src_file_size), "");
    if (remain_size == -1) {
        remain_size = src_file_size;
    }
    remain_size = std::min(remain_size, src_file_size);

    rocksdb::EnvOptions wt_env_options;
    std::unique_ptr<rocksdb::WritableFile> wfile;
    s = dsn::utils::PegasusEnv(dst_type)->NewWritableFile(dst_fname, &wfile, wt_env_options);
    LOG_AND_RETURN_NOT_RDB_OK(WARNING, s, "failed to open file {} for writing", dst_fname);

    // Read at most 4MB once.
    // TODO(yingchun): make it configurable.
    const uint64_t kBlockSize = 4 << 20;
    auto buffer = dsn::utils::make_shared_array<char>(kBlockSize);
    uint64_t offset = 0;
    do {
        int bytes_per_copy = std::min(remain_size, static_cast<int64_t>(kBlockSize));
        if (bytes_per_copy <= 0) {
            break;
        }

        rocksdb::Slice result;
        LOG_AND_RETURN_NOT_RDB_OK(WARNING,
                                  sfile->Read(bytes_per_copy, &result, buffer.get()),
                                  "failed to read file {}",
                                  src_fname);
        CHECK(!result.empty(),
              "read file {} at offset {} with size {} failed",
              src_fname,
              offset,
              bytes_per_copy);
        LOG_AND_RETURN_NOT_RDB_OK(
            WARNING, wfile->Append(result), "failed to write file {}", dst_fname);

        offset += result.size();
        remain_size -= result.size();

        // Reach the end of the file.
        if (result.size() < bytes_per_copy) {
            break;
        }
    } while (true);
    LOG_AND_RETURN_NOT_RDB_OK(WARNING, wfile->Fsync(), "failed to fsync file {}", dst_fname);

    if (total_size != nullptr) {
        *total_size = offset;
    }

    LOG_INFO("copy file from {} to {}, total size {}", src_fname, dst_fname, offset);
    return rocksdb::Status::OK();
}

rocksdb::Status
copy_file(const std::string &src_fname, const std::string &dst_fname, uint64_t *total_size)
{
    // TODO(yingchun): Consider to use hard link, LinkFile().
    return do_copy_file(
        src_fname, FileDataType::kSensitive, dst_fname, FileDataType::kSensitive, -1, total_size);
}

rocksdb::Status
encrypt_file(const std::string &src_fname, const std::string &dst_fname, uint64_t *total_size)
{
    return do_copy_file(src_fname,
                        FileDataType::kNonSensitive,
                        dst_fname,
                        FileDataType::kSensitive,
                        -1,
                        total_size);
}

rocksdb::Status encrypt_file(const std::string &fname, uint64_t *total_size)
{
    // TODO(yingchun): add timestamp to the tmp encrypted file name.
    std::string tmp_fname = fname + ".encrypted.tmp";
    LOG_AND_RETURN_NOT_RDB_OK(
        WARNING, encrypt_file(fname, tmp_fname, total_size), "failed to encrypt file {}", fname);
    if (!::dsn::utils::filesystem::rename_path(tmp_fname, fname)) {
        LOG_WARNING("rename file from {} to {} failed", tmp_fname, fname);
        return rocksdb::Status::IOError("rename file failed");
    }
    return rocksdb::Status::OK();
}

rocksdb::Status
copy_file_by_size(const std::string &src_fname, const std::string &dst_fname, int64_t limit_size)
{
    return do_copy_file(src_fname,
                        FileDataType::kSensitive,
                        dst_fname,
                        FileDataType::kSensitive,
                        limit_size,
                        nullptr);
}

} // namespace utils
} // namespace dsn
