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

#include "test_util.h"

#include <gtest/gtest-spi.h>
#include <rocksdb/env.h>
#include <stdint.h>
#include <chrono>
#include <memory>
#include <ostream>
#include <thread>

#include "gtest/gtest-message.h"
#include "gtest/gtest-test-part.h"
#include "gtest/gtest.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "runtime/api_layer1.h"
#include "utils/defer.h"
#include "utils/encryption_utils.h"
#include "utils/fmt_logging.h"
#include "utils/utils.h"
#include "utils/filesystem.h"

namespace pegasus {

void encrypt_file(const std::string &src, const std::string &dst)
{
    std::unique_ptr<rocksdb::SequentialFile> sfile;
    auto s = dsn::utils::PegasusEnv(dsn::utils::FileDataType::kNonSensitive)
                 ->NewSequentialFile(src, &sfile, rocksdb::EnvOptions());
    ASSERT_TRUE(s.ok()) << s.ToString();

    std::unique_ptr<rocksdb::WritableFile> wfile;
    s = dsn::utils::PegasusEnv(dsn::utils::FileDataType::kSensitive)
            ->NewWritableFile(dst, &wfile, rocksdb::EnvOptions());
    ASSERT_TRUE(s.ok()) << s.ToString();

    const uint64_t kBlockSize = 4 << 20;
    auto buffer = dsn::utils::make_shared_array<char>(kBlockSize);
    uint64_t total_size = 0;
    do {
        // Read 4MB at a time.
        rocksdb::Slice result;
        s = sfile->Read(kBlockSize, &result, buffer.get());
        ASSERT_TRUE(s.ok()) << s.ToString();
        if (result.empty()) {
            break;
        }

        s = wfile->Append(result);
        ASSERT_TRUE(s.ok()) << s.ToString();
        total_size += result.size();

        if (result.size() < kBlockSize) {
            break;
        }
    } while (true);

    LOG_INFO("encrypt file from {} to {}, total size {}", src, dst, total_size);
}

void encrypt_file(const std::string &fname)
{
    // TODO(yingchun): add timestamp to the tmp encrypted file name.
    std::string tmp_fname = fname + ".encrypted.tmp";
    NO_FATALS(encrypt_file(fname, tmp_fname));
    ASSERT_TRUE(::dsn::utils::filesystem::rename_path(tmp_fname, fname));
}

void AssertEventually(const std::function<void(void)> &f, int timeout_sec, WaitBackoff backoff)
{
    // TODO(yingchun): should use mono time
    uint64_t deadline = dsn_now_s() + timeout_sec;
    {
        // Disable gtest's "on failure" behavior, or else the assertion failures
        // inside our attempts will cause the test to end even though we would
        // like to retry.
        bool old_break_on_failure = testing::FLAGS_gtest_break_on_failure;
        bool old_throw_on_failure = testing::FLAGS_gtest_throw_on_failure;
        auto c = dsn::defer([old_break_on_failure, old_throw_on_failure]() {
            testing::FLAGS_gtest_break_on_failure = old_break_on_failure;
            testing::FLAGS_gtest_throw_on_failure = old_throw_on_failure;
        });
        testing::FLAGS_gtest_break_on_failure = false;
        testing::FLAGS_gtest_throw_on_failure = false;

        for (int attempts = 0; dsn_now_s() < deadline; attempts++) {
            // Capture any assertion failures within this scope (i.e. from their function)
            // into 'results'
            testing::TestPartResultArray results;
            testing::ScopedFakeTestPartResultReporter reporter(
                testing::ScopedFakeTestPartResultReporter::INTERCEPT_ONLY_CURRENT_THREAD, &results);
            f();

            // Determine whether their function produced any new test failure results.
            bool has_failures = false;
            for (int i = 0; i < results.size(); i++) {
                has_failures |= results.GetTestPartResult(i).failed();
            }
            if (!has_failures) {
                return;
            }

            // If they had failures, sleep and try again.
            int sleep_ms = 0;
            switch (backoff) {
            case WaitBackoff::EXPONENTIAL:
                sleep_ms = (attempts < 10) ? (1 << attempts) : 1000;
                break;
            case WaitBackoff::NONE:
                sleep_ms = 1000;
                break;
            default:
                LOG_FATAL("Unknown backoff type");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        }
    }

    // If we ran out of time looping, run their function one more time
    // without capturing its assertions. This way the assertions will
    // propagate back out to the normal test reporter. Of course it's
    // possible that it will pass on this last attempt, but that's OK
    // too, since we aren't trying to be that strict about the deadline.
    f();
    if (testing::Test::HasFatalFailure()) {
        ADD_FAILURE() << "Timed out waiting for assertion to pass.";
    }
}

void WaitCondition(const std::function<bool(void)> &f, int timeout_sec, WaitBackoff backoff)
{
    uint64_t deadline = dsn_now_s() + timeout_sec;
    for (int attempts = 0; dsn_now_s() < deadline; attempts++) {
        if (f()) {
            break;
        }
        int sleep_ms = 0;
        switch (backoff) {
        case WaitBackoff::EXPONENTIAL:
            sleep_ms = (attempts < 10) ? (1 << attempts) : 1000;
            break;
        case WaitBackoff::NONE:
            sleep_ms = 1000;
            break;
        default:
            LOG_FATAL("Unknown backoff type");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
}

} // namespace pegasus
