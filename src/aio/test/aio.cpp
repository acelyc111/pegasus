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

#include <alloca.h>
#include <fcntl.h>
// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
#include <gtest/gtest.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <list>
#include <memory>
#include <random>
#include <string>

#include "aio/aio_task.h"
#include "aio/file_io.h"
#include "runtime/task/task_code.h"
#include "runtime/tool_api.h"
#include "test_util/test_util.h"
#include "utils/autoref_ptr.h"
#include "utils/error_code.h"
#include "utils/fail_point.h"
#include "utils/filesystem.h"
#include "utils/flags.h"
#include "utils/fmt_logging.h"
#include "utils/ports.h"
#include "utils/strings.h"
#include "utils/test_macros.h"
#include "utils/threadpool_code.h"

namespace dsn {
class disk_file;
} // namespace dsn

using namespace ::dsn;

DEFINE_THREAD_POOL_CODE(THREAD_POOL_TEST_SERVER)
DEFINE_TASK_CODE_AIO(LPC_AIO_TEST, TASK_PRIORITY_COMMON, THREAD_POOL_TEST_SERVER);

class aio_test : public pegasus::encrypt_data_test_base
{
public:
    void SetUp() override { utils::filesystem::remove_path(_test_file_name); }

    void TearDown() override { utils::filesystem::remove_path(_test_file_name); }

    std::string _test_file_name = "tmp";
};

INSTANTIATE_TEST_CASE_P(, aio_test, ::testing::Values(false, true));

TEST_P(aio_test, basic)
{
    const char *kUnitBuffer = "hello, world";
    size_t kUnitBufferLength = strlen(kUnitBuffer);
    int kTotalBufferCount = 100;
    int kBufferCountPerBatch = 10;
    ASSERT_EQ(0, kTotalBufferCount % kBufferCountPerBatch);

    // write file
    auto wfile = file::open(_test_file_name, file::FileOpenType::kWriteOnly);
    ASSERT_NE(wfile, nullptr);

    // read file
    auto rfile = file::open(_test_file_name, file::FileOpenType::kReadOnly);
    ASSERT_NE(rfile, nullptr);

    auto check_callback = [kUnitBufferLength](::dsn::error_code err, size_t n) {
        CHECK_EQ(ERR_OK, err);
        CHECK_EQ(kUnitBufferLength, n);
    };
    auto verify_data = [=]() {
        // sequential read
        {
            uint64_t offset = 0;
            std::list<aio_task_ptr> tasks;
            for (int i = 0; i < kTotalBufferCount; i++) {
                char read_buffer[kUnitBufferLength + 1];
                read_buffer[kUnitBufferLength] = 0;
                auto t = ::dsn::file::read(rfile,
                                           read_buffer,
                                           kUnitBufferLength,
                                           offset,
                                           LPC_AIO_TEST,
                                           nullptr,
                                           check_callback);
                offset += kUnitBufferLength;

                t->wait();
                ASSERT_EQ(kUnitBufferLength, t->get_transferred_size());
                if (strcmp(kUnitBuffer, read_buffer) != 0) {
                    assert(false);
                }
                ASSERT_STREQ(kUnitBuffer, read_buffer) << i;
            }
        }

        // concurrent read
        {
            uint64_t offset = 0;
            std::list<aio_task_ptr> tasks;
            char read_buffers[kTotalBufferCount][kUnitBufferLength + 1];
            for (int i = 0; i < kTotalBufferCount; i++) {
                read_buffers[i][kUnitBufferLength] = 0;
                auto t = ::dsn::file::read(rfile,
                                           read_buffers[i],
                                           kUnitBufferLength,
                                           offset,
                                           LPC_AIO_TEST,
                                           nullptr,
                                           check_callback);
                offset += kUnitBufferLength;
                tasks.push_back(t);
            }
            for (auto &t : tasks) {
                t->wait();
                ASSERT_EQ(kUnitBufferLength, t->get_transferred_size());
            }
            for (int i = 0; i < kTotalBufferCount; i++) {
                if (strcmp(kUnitBuffer, read_buffers[i]) != 0) {
                    ASSERT_STREQ(kUnitBuffer, read_buffers[i]) << i;
                }
                ASSERT_STREQ(kUnitBuffer, read_buffers[i]) << i;
            }
        }
    };

    // new write
    {
        uint64_t offset = 0;
        std::list<aio_task_ptr> tasks;
        for (int i = 0; i < kTotalBufferCount; i++) {
            auto t = ::dsn::file::write(wfile,
                                        kUnitBuffer,
                                        kUnitBufferLength,
                                        offset,
                                        LPC_AIO_TEST,
                                        nullptr,
                                        check_callback);
            offset += kUnitBufferLength;
            tasks.push_back(t);
        }
        for (auto &t : tasks) {
            t->wait();
            ASSERT_EQ(kUnitBufferLength, t->get_transferred_size());
        }
        ASSERT_EQ(ERR_OK, file::flush(wfile));
    }
    NO_FATALS(verify_data());

    // un-sequential write
    {
        std::vector<uint64_t> offsets;
        offsets.reserve(kTotalBufferCount);
        for (int i = 0; i < kTotalBufferCount; i++) {
            offsets.push_back(i * kUnitBufferLength);
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(offsets.begin(), offsets.end(), gen);

        std::list<aio_task_ptr> tasks;
        for (const auto &offset : offsets) {
            auto t = ::dsn::file::write(wfile,
                                        kUnitBuffer,
                                        kUnitBufferLength,
                                        offset,
                                        LPC_AIO_TEST,
                                        nullptr,
                                        check_callback);
            tasks.push_back(t);
        }
        for (auto &t : tasks) {
            t->wait();
            ASSERT_EQ(kUnitBufferLength, t->get_transferred_size());
        }
        ASSERT_EQ(ERR_OK, file::flush(wfile));
    }
    NO_FATALS(verify_data());

    // overwrite
    {
        uint64_t offset = 0;
        std::list<aio_task_ptr> tasks;
        for (int i = 0; i < kTotalBufferCount; i++) {
            auto t = ::dsn::file::write(wfile,
                                        kUnitBuffer,
                                        kUnitBufferLength,
                                        offset,
                                        LPC_AIO_TEST,
                                        nullptr,
                                        check_callback);
            offset += kUnitBufferLength;
            tasks.push_back(t);
        }
        for (auto &t : tasks) {
            t->wait();
            ASSERT_EQ(kUnitBufferLength, t->get_transferred_size());
        }
        ASSERT_EQ(ERR_OK, file::flush(wfile));
    }
    NO_FATALS(verify_data());

    // vector write
    {
        uint64_t offset = 0;
        std::list<aio_task_ptr> tasks;
        std::unique_ptr<dsn_file_buffer_t[]> buffers(new dsn_file_buffer_t[kBufferCountPerBatch]);
        for (int i = 0; i < kBufferCountPerBatch; i++) {
            buffers[i].buffer = static_cast<void *>(const_cast<char *>(kUnitBuffer));
            buffers[i].size = kUnitBufferLength;
        }
        for (int i = 0; i < kTotalBufferCount / kBufferCountPerBatch; i++) {
            tasks.push_back(
                ::dsn::file::write_vector(wfile,
                                          buffers.get(),
                                          kBufferCountPerBatch,
                                          offset,
                                          LPC_AIO_TEST,
                                          nullptr,
                                          [=](::dsn::error_code err, size_t n) {
                                              CHECK_EQ(ERR_OK, err);
                                              CHECK_EQ(kBufferCountPerBatch * kUnitBufferLength, n);
                                          }));
            offset += kBufferCountPerBatch * kUnitBufferLength;
        }
        for (auto &t : tasks) {
            t->wait();
            ASSERT_EQ(kBufferCountPerBatch * kUnitBufferLength, t->get_transferred_size());
        }
        ASSERT_EQ(ERR_OK, file::flush(wfile));
    }
    NO_FATALS(verify_data());
    ASSERT_EQ(ERR_OK, file::close(wfile));
}

TEST_P(aio_test, aio_share)
{
    auto wfile = file::open(_test_file_name, file::FileOpenType::kWriteOnly);
    ASSERT_NE(wfile, nullptr);

    auto rfile = file::open(_test_file_name, file::FileOpenType::kReadOnly);
    ASSERT_NE(rfile, nullptr);

    file::close(wfile);
    file::close(rfile);
}

TEST_P(aio_test, operation_failed)
{
    auto err = std::make_unique<dsn::error_code>();
    auto count = std::make_unique<size_t>();
    auto io_callback = [&err, &count](::dsn::error_code e, size_t n) {
        *err = e;
        *count = n;
    };

    auto wfile = file::open(_test_file_name, file::FileOpenType::kWriteOnly);
    ASSERT_NE(wfile, nullptr);

    char buff[512] = {0};
    const char *kUnitBuffer = "hello file";
    const size_t kUnitBufferLength = strlen(kUnitBuffer);
    auto t = ::dsn::file::write(
        wfile, kUnitBuffer, kUnitBufferLength, 0, LPC_AIO_TEST, nullptr, io_callback, 0);
    t->wait();
    ASSERT_EQ(ERR_OK, *err);
    ASSERT_EQ(kUnitBufferLength, *count);

    t = ::dsn::file::read(wfile, buff, 512, 0, LPC_AIO_TEST, nullptr, io_callback, 0);
    t->wait();
    ASSERT_EQ(ERR_FILE_OPERATION_FAILED, *err);

    auto rfile = file::open(_test_file_name, file::FileOpenType::kReadOnly);
    ASSERT_NE(nullptr, rfile);

    t = ::dsn::file::read(rfile, buff, 512, 0, LPC_AIO_TEST, nullptr, io_callback, 0);
    t->wait();
    ASSERT_EQ(ERR_OK, *err);
    ASSERT_EQ(kUnitBufferLength, *count);
    ASSERT_STREQ(kUnitBuffer, buff);

    t = ::dsn::file::read(rfile, buff, 5, 0, LPC_AIO_TEST, nullptr, io_callback, 0);
    t->wait();
    ASSERT_EQ(ERR_OK, *err);
    ASSERT_EQ(5, *count);
    ASSERT_STREQ(kUnitBuffer, buff);

    t = ::dsn::file::read(rfile, buff, 512, 100, LPC_AIO_TEST, nullptr, io_callback, 0);
    t->wait();
    LOG_INFO("error code: {}", *err);
    file::close(wfile);
    file::close(rfile);
}

DEFINE_TASK_CODE_AIO(LPC_AIO_TEST_READ, TASK_PRIORITY_COMMON, THREAD_POOL_DEFAULT)
DEFINE_TASK_CODE_AIO(LPC_AIO_TEST_WRITE, TASK_PRIORITY_COMMON, THREAD_POOL_DEFAULT)
struct aio_result
{
    dsn::error_code err;
    size_t sz;
};

TEST_P(aio_test, dsn_file)
{
    if (FLAGS_encrypt_data_at_rest) {
        // The testfiles copy_source.txt and copy_dest.txt are not encrypted.
        return;
    }

    int64_t fin_size, fout_size;
    ASSERT_TRUE(utils::filesystem::file_size(
        "copy_source.txt", utils::filesystem::FileDataType::kNonSensitive, fin_size));
    ASSERT_LT(0, fin_size);

    dsn::disk_file *fin = file::open("copy_source.txt", file::FileOpenType::kReadOnly);
    ASSERT_NE(nullptr, fin);
    dsn::disk_file *fout = file::open("copy_dest.txt", file::FileOpenType::kWriteOnly);
    ASSERT_NE(nullptr, fout);
    char kUnitBuffer[1024];
    uint64_t offset = 0;
    while (true) {
        aio_result rin;
        aio_task_ptr tin = file::read(fin,
                                      kUnitBuffer,
                                      1024,
                                      offset,
                                      LPC_AIO_TEST_READ,
                                      nullptr,
                                      [&rin](dsn::error_code err, size_t sz) {
                                          rin.err = err;
                                          rin.sz = sz;
                                      },
                                      0);
        ASSERT_NE(nullptr, tin);

        if (dsn::tools::get_current_tool()->name() != "simulator") {
            // at least 1 for tin, but if already read completed, then only 1
            ASSERT_LE(1, tin->get_count());
        }

        tin->wait();
        ASSERT_EQ(rin.err, tin->error());
        if (rin.err != ERR_OK) {
            ASSERT_EQ(ERR_HANDLE_EOF, rin.err);
            break;
        }
        ASSERT_LT(0u, rin.sz);
        ASSERT_EQ(rin.sz, tin->get_transferred_size());
        // this is only true for simulator
        if (dsn::tools::get_current_tool()->name() == "simulator") {
            ASSERT_EQ(1, tin->get_count());
        }

        aio_result rout;
        aio_task_ptr tout = file::write(fout,
                                        kUnitBuffer,
                                        rin.sz,
                                        offset,
                                        LPC_AIO_TEST_WRITE,
                                        nullptr,
                                        [&rout](dsn::error_code err, size_t sz) {
                                            rout.err = err;
                                            rout.sz = sz;
                                        },
                                        0);
        ASSERT_NE(nullptr, tout);
        tout->wait();
        ASSERT_EQ(ERR_OK, rout.err);
        ASSERT_EQ(ERR_OK, tout->error());
        ASSERT_EQ(rin.sz, rout.sz);
        ASSERT_EQ(rin.sz, tout->get_transferred_size());
        // this is only true for simulator
        if (dsn::tools::get_current_tool()->name() == "simulator") {
            ASSERT_EQ(1, tout->get_count());
        }

        ASSERT_EQ(ERR_OK, file::flush(fout));

        offset += rin.sz;
    }

    ASSERT_EQ((uint64_t)fin_size, offset);
    ASSERT_EQ(ERR_OK, file::close(fout));
    ASSERT_EQ(ERR_OK, file::close(fin));

    ASSERT_TRUE(utils::filesystem::file_size(
        "copy_dest.txt", utils::filesystem::FileDataType::kNonSensitive, fout_size));
    ASSERT_EQ(fin_size, fout_size);
}
