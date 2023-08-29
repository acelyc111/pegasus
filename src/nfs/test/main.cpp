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

// IWYU pragma: no_include <gtest/gtest-param-test.h>
// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
#include <gtest/gtest.h>
#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "aio/aio_task.h"
#include "common/gpid.h"
#include "nfs/nfs_node.h"
#include "runtime/app_model.h"
#include "runtime/rpc/rpc_address.h"
#include "runtime/task/task_code.h"
#include "runtime/tool_api.h"
#include "test_util/test_util.h"
#include "utils/autoref_ptr.h"
#include "utils/encryption_utils.h"
#include "utils/error_code.h"
#include "utils/filesystem.h"
#include "utils/flags.h"
#include "utils/threadpool_code.h"

DSN_DECLARE_bool(encrypt_data_at_rest);

using namespace dsn;

DEFINE_TASK_CODE_AIO(LPC_AIO_TEST_NFS, TASK_PRIORITY_COMMON, THREAD_POOL_DEFAULT)
struct aio_result
{
    dsn::error_code err;
    size_t sz;
};

class nfs_test : public pegasus::encrypt_data_test_base
{
};

INSTANTIATE_TEST_CASE_P(, nfs_test, ::testing::Values(false, true));

TEST_P(nfs_test, basic)
{
    auto nfs = dsn::nfs_node::create();
    nfs->start();
    nfs->register_async_rpc_handler_for_test();
    dsn::gpid fake_pid = gpid(1, 0);

    // Prepare the destination directory.
    std::string dst_dir = "nfs_test_dir";
    ASSERT_TRUE(utils::filesystem::remove_path(dst_dir));
    ASSERT_FALSE(utils::filesystem::directory_exists(dst_dir));
    ASSERT_TRUE(utils::filesystem::create_directory(dst_dir));
    ASSERT_TRUE(utils::filesystem::directory_exists(dst_dir));

    // Prepare the source files information.
    std::vector<std::string> src_filenames({"nfs_test_file1", "nfs_test_file2"});
    if (FLAGS_encrypt_data_at_rest) {
        for (int i = 0; i < src_filenames.size(); i++) {
            encrypt_file(src_filenames[i], src_filenames[i] + ".encrypted");
            src_filenames[i] += ".encrypted";
        }
    }
    std::vector<int64_t> src_file_sizes;
    std::vector<std::string> src_file_md5s;
    for (const auto &src_filename : src_filenames) {
        int64_t file_size;
        ASSERT_TRUE(utils::filesystem::file_size(
            src_filename, dsn::utils::FileDataType::kSensitive, file_size));
        src_file_sizes.push_back(file_size);
        std::string src_file_md5;
        ASSERT_EQ(ERR_OK, utils::filesystem::md5sum(src_filename, src_file_md5));
        src_file_md5s.emplace_back(std::move(src_file_md5));
    }

    // copy files to the destination directory.
    {
        // The destination directory is empty before copying.
        std::vector<std::string> dst_filenames;
        ASSERT_TRUE(utils::filesystem::get_subfiles(dst_dir, dst_filenames, true));
        ASSERT_TRUE(dst_filenames.empty());

        aio_result r;
        dsn::aio_task_ptr t = nfs->copy_remote_files(dsn::rpc_address("localhost", 20101),
                                                     "default",
                                                     ".",
                                                     src_filenames,
                                                     "default",
                                                     dst_dir,
                                                     fake_pid,
                                                     false,
                                                     false,
                                                     LPC_AIO_TEST_NFS,
                                                     nullptr,
                                                     [&r](dsn::error_code err, size_t sz) {
                                                         r.err = err;
                                                         r.sz = sz;
                                                     },
                                                     0);
        ASSERT_NE(nullptr, t);
        ASSERT_TRUE(t->wait(20000));
        ASSERT_EQ(r.err, t->error());
        ASSERT_EQ(ERR_OK, r.err);
        ASSERT_EQ(r.sz, t->get_transferred_size());

        // The destination files equal to the source files after copying.
        ASSERT_TRUE(utils::filesystem::get_subfiles(dst_dir, dst_filenames, true));
        std::sort(dst_filenames.begin(), dst_filenames.end());
        ASSERT_EQ(src_filenames.size(), dst_filenames.size());
        int i = 0;
        for (const auto &dst_filename : dst_filenames) {
            int64_t file_size;
            ASSERT_TRUE(utils::filesystem::file_size(
                dst_filename, dsn::utils::FileDataType::kSensitive, file_size));
            ASSERT_EQ(src_file_sizes[i], file_size);
            std::string file_md5;
            ASSERT_EQ(ERR_OK, utils::filesystem::md5sum(dst_filename, file_md5));
            ASSERT_EQ(src_file_md5s[i], file_md5);
            i++;
        }
    }

    // copy files to the destination directory, files will be overwritten.
    {
        aio_result r;
        dsn::aio_task_ptr t = nfs->copy_remote_files(dsn::rpc_address("localhost", 20101),
                                                     "default",
                                                     ".",
                                                     src_filenames,
                                                     "default",
                                                     dst_dir,
                                                     fake_pid,
                                                     true,
                                                     false,
                                                     LPC_AIO_TEST_NFS,
                                                     nullptr,
                                                     [&r](dsn::error_code err, size_t sz) {
                                                         r.err = err;
                                                         r.sz = sz;
                                                     },
                                                     0);
        ASSERT_NE(nullptr, t);
        ASSERT_TRUE(t->wait(20000));
        ASSERT_EQ(r.err, t->error());
        ASSERT_EQ(ERR_OK, r.err);
        ASSERT_EQ(r.sz, t->get_transferred_size());

        // this is only true for simulator
        if (dsn::tools::get_current_tool()->name() == "simulator") {
            ASSERT_EQ(1, t->get_count());
        }

        // The destination files equal to the source files after overwrite copying.
        std::vector<std::string> dst_filenames;
        ASSERT_TRUE(utils::filesystem::get_subfiles(dst_dir, dst_filenames, true));
        std::sort(dst_filenames.begin(), dst_filenames.end());
        ASSERT_EQ(src_filenames.size(), dst_filenames.size());
        int i = 0;
        for (const auto &dst_filename : dst_filenames) {
            int64_t file_size;
            ASSERT_TRUE(utils::filesystem::file_size(
                dst_filename, dsn::utils::FileDataType::kSensitive, file_size));
            ASSERT_EQ(src_file_sizes[i], file_size);
            std::string file_md5;
            ASSERT_EQ(ERR_OK, utils::filesystem::md5sum(dst_filename, file_md5));
            ASSERT_EQ(src_file_md5s[i], file_md5);
            i++;
        }
    }

    // copy files from dst_dir to new_dst_dir.
    {
        std::string new_dst_dir = "nfs_test_dir_copy";
        ASSERT_TRUE(utils::filesystem::remove_path(new_dst_dir));
        ASSERT_FALSE(utils::filesystem::directory_exists(new_dst_dir));

        aio_result r;
        dsn::aio_task_ptr t = nfs->copy_remote_directory(dsn::rpc_address("localhost", 20101),
                                                         "default",
                                                         dst_dir,
                                                         "default",
                                                         new_dst_dir,
                                                         fake_pid,
                                                         false,
                                                         false,
                                                         LPC_AIO_TEST_NFS,
                                                         nullptr,
                                                         [&r](dsn::error_code err, size_t sz) {
                                                             r.err = err;
                                                             r.sz = sz;
                                                         },
                                                         0);
        ASSERT_NE(nullptr, t);
        ASSERT_TRUE(t->wait(20000));
        ASSERT_EQ(r.err, t->error());
        ASSERT_EQ(ERR_OK, r.err);
        ASSERT_EQ(r.sz, t->get_transferred_size());

        // The new_dst_dir will be created automatically.
        ASSERT_TRUE(utils::filesystem::directory_exists(new_dst_dir));

        std::vector<std::string> new_dst_filenames;
        ASSERT_TRUE(utils::filesystem::get_subfiles(new_dst_dir, new_dst_filenames, true));
        std::sort(new_dst_filenames.begin(), new_dst_filenames.end());
        ASSERT_EQ(src_filenames.size(), new_dst_filenames.size());

        int i = 0;
        for (const auto &new_dst_filename : new_dst_filenames) {
            int64_t file_size;
            ASSERT_TRUE(utils::filesystem::file_size(
                new_dst_filename, dsn::utils::FileDataType::kSensitive, file_size));
            ASSERT_EQ(src_file_sizes[i], file_size);
            std::string file_md5;
            ASSERT_EQ(ERR_OK, utils::filesystem::md5sum(new_dst_filename, file_md5));
            ASSERT_EQ(src_file_md5s[i], file_md5);
            i++;
        }
    }

    nfs->stop();
}

int g_test_ret = 0;
GTEST_API_ int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    dsn_run_config("config.ini", false);
    g_test_ret = RUN_ALL_TESTS();
    dsn_exit(g_test_ret);
}
