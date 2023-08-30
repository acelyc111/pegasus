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

// IWYU pragma: no_include <gtest/gtest-param-test.h>
// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
#include <gtest/gtest.h>
#include <stdint.h>
#include <string>

#include <rocksdb/env.h>

#include "test_util/test_util.h"
#include "utils/encryption_utils.h"
#include "utils/filesystem.h"

namespace dsn {
namespace utils {
namespace filesystem {

// The old filesystem API doesn't support sensitive files, so skip testing
// FLAGS_encrypt_data_at_rest=true.

TEST(filesystem_test, check_new_md5sum)
{
    FLAGS_encrypt_data_at_rest = false;

    struct file_info
    {
        int64_t size;
    } tests[]{{4095}, {4096}, {4097}};

    for (const auto &test : tests) {
        std::string fname = "test_file";
        // deprecated_md5sum doesn't support kSensitive files, so use kNonSensitive here.
        auto s = rocksdb::WriteStringToFile(
            dsn::utils::PegasusEnv(dsn::utils::FileDataType::kNonSensitive),
            rocksdb::Slice(std::string(test.size, 'a')),
            fname,
            /* should_sync */ true);
        ASSERT_TRUE(s.ok()) << s.ToString();
        // Check the file size.
        int64_t file_fsize;
        ASSERT_TRUE(file_size(fname, FileDataType::kNonSensitive, file_fsize));
        ASSERT_EQ(test.size, file_fsize);

        // Get the md5sum.
        std::string md5sum1;
        ASSERT_EQ(ERR_OK, md5sum(fname, md5sum1));
        ASSERT_FALSE(md5sum1.empty());

        // Check the md5sum is repeatable.
        std::string md5sum2;
        ASSERT_EQ(ERR_OK, md5sum(fname, md5sum2));
        ASSERT_EQ(md5sum1, md5sum2);

        // Check the md5sum is the same to deprecated_md5sum.
        ASSERT_EQ(ERR_OK, deprecated_md5sum(fname, md5sum2));
        ASSERT_EQ(md5sum1, md5sum2);

        utils::filesystem::remove_path(fname);
    }
}

TEST(filesystem_test, verify_file_test)
{
    FLAGS_encrypt_data_at_rest = false;

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
