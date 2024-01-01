/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Microsoft Corporation
 *
 * -=- Robust Distributed System Nucleus (rDSN) -=-
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
#include <chrono>
#include <string>
#include <utility>

#include "gtest/gtest.h"
#include "rpc/rpc_address.h"
#include "runtime/task/async_calls.h"
#include "runtime/test_utils.h"
#include "utils/crc.h"
#include "utils/error_code.h"
#include "utils/rand.h"

TEST(corrupt_message_test, crc)
{
    char buffer[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    auto c1 = dsn::utils::crc32_calc(buffer, 18, 0);
    auto c2 = dsn::utils::crc32_calc(buffer + 18, 18, c1);
    auto c3 = dsn::utils::crc32_calc(buffer, 36, 0);
    auto c4 = dsn::utils::crc32_concat(0, 0, c1, 18, c1, c2, 18);
    ASSERT_EQ(c3, c4);

    auto c5 = dsn::utils::crc32_calc(buffer, 36, 0);
    buffer[10]++;
    auto c6 = dsn::utils::crc32_calc(buffer, 36, 0);
    std::cout << c5 << std::endl;
    std::cout << c6 << std::endl;
}

// this only works with the fault injector
TEST(corrupt_message_test, basic)
{
    ::dsn::rpc_address server("localhost", 20101);

    std::vector<task_code> codes({RPC_TEST_HASH1, RPC_TEST_HASH2, RPC_TEST_HASH3, RPC_TEST_HASH4});
    for (const auto &code : codes) {
        auto result = ::dsn::rpc::call_wait<std::string>(
            server, code, 0, std::chrono::milliseconds(0), 1);
        ASSERT_EQ(ERR_TIMEOUT, result.first);
    }
}
