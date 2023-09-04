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

#include "encryption_utils.h"

#include <fmt/core.h>
#include <rocksdb/convenience.h>
#include <rocksdb/env.h>
#include <rocksdb/env_encryption.h>
#include <rocksdb/status.h>
#include <memory>
#include <string>

#include "utils/flags.h"
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
    if (FLAGS_encrypt_data_at_rest) {
        static rocksdb::Env *env = NewEncryptedEnv();
        return env;
    }

    static rocksdb::Env *env = rocksdb::Env::Default();
    return env;
}
} // namespace dsn
} // namespace utils
