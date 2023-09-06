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
#include <rocksdb/env_encryption.h>
#include <memory>
#include <string>

#include "utils/flags.h"
#include "utils/filesystem.h"
#include "utils/fmt_logging.h"

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

DSN_DEFINE_bool(replication,
                enable_direct_io,
                false,
                "Whether to enable direct I/O when download files");
DSN_TAG_VARIABLE(enable_direct_io, FT_MUTABLE);

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
                             dsn::utils::FileDataType dst_type)
{
    std::unique_ptr<rocksdb::SequentialFile> sfile;
    auto s = dsn::utils::PegasusEnv(src_type)->NewSequentialFile(
        src_fname, &sfile, rocksdb::EnvOptions());
    LOG_AND_RETURN_NOT_RDB_OK(WARNING, s, "failed to open file {} for reading", src_fname);

    std::unique_ptr<rocksdb::WritableFile> wfile;
    s = dsn::utils::PegasusEnv(dst_type)->NewWritableFile(dst_fname, &wfile, rocksdb::EnvOptions());
    LOG_AND_RETURN_NOT_RDB_OK(WARNING, s, "failed to open file {} for writing", dst_fname);

    const uint64_t kBlockSize = 4 << 20;
    auto buffer = dsn::utils::make_shared_array<char>(kBlockSize);
    uint64_t total_size = 0;
    do {
        // Read 4MB at a time.
        rocksdb::Slice result;
        s = sfile->Read(kBlockSize, &result, buffer.get());
        LOG_AND_RETURN_NOT_RDB_OK(WARNING, s, "failed to read file {}", src_fname);
        if (result.empty()) {
            break;
        }

        s = wfile->Append(result);
        LOG_AND_RETURN_NOT_RDB_OK(WARNING, s, "failed to write file {}", dst_fname);
        total_size += result.size();

        if (result.size() < kBlockSize) {
            break;
        }
    } while (true);

    s = wfile->Fsync();
    if (!s.ok()) {
        LOG_AND_RETURN_NOT_RDB_OK(WARNING, s, "failed to fsync file {}", dst_fname);
    }

    LOG_INFO("copy file from {} to {}, total size {}", src_fname, dst_fname, total_size);
    return rocksdb::Status::OK();
}

rocksdb::Status copy_file(const std::string &src_fname, const std::string &dst_fname)
{
    // All files are encrypted by default.
    return do_copy_file(src_fname, FileDataType::kSensitive, dst_fname, FileDataType::kSensitive);
}

rocksdb::Status encrypt_file(const std::string &src_fname, const std::string &dst_fname)
{
    return do_copy_file(
        src_fname, FileDataType::kNonSensitive, dst_fname, FileDataType::kSensitive);
}

rocksdb::Status encrypt_file(const std::string &fname)
{
    // TODO(yingchun): add timestamp to the tmp encrypted file name.
    std::string tmp_fname = fname + ".encrypted.tmp";
    LOG_AND_RETURN_NOT_RDB_OK(
        WARNING, encrypt_file(fname, tmp_fname), "failed to encrypt file {}", fname);
    if (!::dsn::utils::filesystem::rename_path(tmp_fname, fname)) {
        LOG_WARNING("rename file from {} to {} failed", tmp_fname, fname);
        return rocksdb::Status::IOError("rename file failed");
    }
    return rocksdb::Status::OK();
}

} // namespace utils
} // namespace dsn
