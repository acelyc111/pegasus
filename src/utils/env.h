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

#pragma once

#include <rocksdb/env.h>
#include <rocksdb/status.h>

#include <string>

namespace dsn {
namespace utils {

// Indicate whether the file is sensitive or not.
// Only the sensitive file will be encrypted if FLAGS_encrypt_data_at_rest
// is enabled at the same time.
enum class FileDataType
{
    kSensitive = 0,
    kNonSensitive = 1
};

rocksdb::Env *PegasusEnv(FileDataType type);

// The 'total_size' is the total size of the file content, exclude the file encryption header.
// 'src_fname' is not encrypted and 'dst_fname' is encrypted.
rocksdb::Status encrypt_file(const std::string &src_fname,
                             const std::string &dst_fname,
                             uint64_t *total_size = nullptr);
// Encrypt the original non-encrypted 'fname'.
rocksdb::Status encrypt_file(const std::string &fname, uint64_t *total_size = nullptr);
// Both 'src_fname' and 'dst_fname' are encrypted files.
rocksdb::Status copy_file(const std::string &src_fname,
                          const std::string &dst_fname,
                          uint64_t *total_size = nullptr);

} // namespace utils
} // namespace dsn
