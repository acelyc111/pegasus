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

#include <gtest/gtest-param-test.h>
// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
#include <gtest/gtest.h>
#include <stdint.h>
#include <string>

#include "test_util/test_util.h"
#include "utils/encryption_utils.h"
#include "utils/filesystem.h"

namespace dsn {
namespace utils {
namespace filesystem {

class filesystem_test : public pegasus::encrypt_data_test_base
{
};

INSTANTIATE_TEST_CASE_P(, filesystem_test, ::testing::Values(false, true));

TEST_P(filesystem_test, verify_file_test)
{
    const std::string &fname = "test_file";
    std::string expected_md5;
    int64_t expected_fsize;
    create_file(fname);
    md5sum(fname, expected_md5);
    ASSERT_TRUE(file_size(fname, FileDataType::kNonSensitive, expected_fsize));

    ASSERT_TRUE(verify_file(fname, FileDataType::kNonSensitive, expected_md5, expected_fsize));
    ASSERT_FALSE(verify_file(fname, FileDataType::kNonSensitive, "wrong_md5", 10086));
    ASSERT_FALSE(verify_file("file_not_exists", FileDataType::kNonSensitive, "wrong_md5", 10086));

    remove_path(fname);
}

} // namespace filesystem
} // namespace utils
} // namespace dsn
