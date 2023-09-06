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

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "base/pegasus_const.h"
#include "block_service/local/local_service.h"
#include "bulk_load_types.h"
#include "client/replication_ddl_client.h"
#include "include/pegasus/client.h"
#include "include/pegasus/error.h"
#include "meta_admin_types.h"
#include "metadata_types.h"
#include "meta/meta_bulk_load_service.h"
#include "rocksdb/env.h"
#include "test/function_test/utils/test_util.h"
#include "test_util/test_util.h"
#include "utils/env.h"
#include "utils/error_code.h"
#include "utils/errors.h"
#include "utils/filesystem.h"
#include "utils/flags.h"
#include "utils/utils.h"

DSN_DECLARE_bool(encrypt_data_at_rest);

using namespace ::dsn;
using namespace ::dsn::replication;
using namespace pegasus;
using std::map;
using std::string;

///
/// Files:
/// `pegasus-bulk-load-function-test-files` folder stores sst files and metadata files used for
/// bulk load function tests
///  - `mock_bulk_load_info` sub-directory stores stores wrong bulk_load_info
///  - `bulk_load_root` sub-directory stores right data
///     - Please do not rename any files or directories under this folder
///
/// The app who is executing bulk load:
/// - app_name is `temp`, app_id is 2, partition_count is 8
///
/// Data:
/// hashkey: hashi sortkey: sorti value: newValue       i=[0, 1000]
/// hashkey: hashkeyj sortkey: sortkeyj value: newValue j=[0, 1000]
///
class bulk_load_test : public test_util
{
protected:
    bulk_load_test() : test_util(map<string, string>({{"rocksdb.allow_ingest_behind", "true"}}))
    {
        TRICKY_CODE_TO_AVOID_LINK_ERROR;
        BULK_LOAD_LOCAL_APP_ROOT =
            fmt::format("{}/{}/{}", LOCAL_BULK_LOAD_ROOT, CLUSTER, app_name_);
    }

    void SetUp() override
    {
        test_util::SetUp();
        NO_FATALS(copy_bulk_load_files());
    }

    void TearDown() override
    {
        ASSERT_EQ(ERR_OK, ddl_client_->drop_app(app_name_, 0));
        NO_FATALS(run_cmd_from_project_root("rm -rf " + LOCAL_BULK_LOAD_ROOT));
    }

    void copy_bulk_load_files()
    {
        // TODO(yingchun): remove
        // src/test/function_test/bulk_load_test/pegasus-bulk-load-function-test-files/mock_bulk_load_info
        // Prepare bulk load files.
        // The source data has 8 partitions.
        ASSERT_EQ(8, partition_count_);
        NO_FATALS(run_cmd_from_project_root("mkdir -p " + LOCAL_BULK_LOAD_ROOT));
        NO_FATALS(run_cmd_from_project_root(
            fmt::format("cp -r {}/{} {}", SOURCE_FILES_ROOT, BULK_LOAD, LOCAL_SERVICE_ROOT)));
        if (FLAGS_encrypt_data_at_rest) {
            std::vector<std::string> src_files;
            ASSERT_TRUE(dsn::utils::filesystem::get_subfiles(LOCAL_SERVICE_ROOT, src_files, true));
            for (const auto &src_file : src_files) {
                auto s = dsn::utils::encrypt_file(src_file);
                ASSERT_TRUE(s.ok()) << s.ToString();
                int64_t file_size;
                ASSERT_TRUE(dsn::utils::filesystem::file_size(
                    src_file, dsn::utils::FileDataType::kNonSensitive, file_size));
                LOG_INFO("get file size of '{}' {}", src_file, file_size);
            }
        }

        // Write file 'bulk_load_info'.
        string bulk_load_info_path =
            fmt::format("{}/cluster/{}/bulk_load_info", LOCAL_BULK_LOAD_ROOT, app_name_);
        {
            bulk_load_info bli;
            bli.app_id = app_id_;
            bli.app_name = app_name_;
            bli.partition_count = partition_count_;
            blob value = dsn::json::json_forwarder<bulk_load_info>::encode(bli);
            auto s = rocksdb::WriteStringToFile(
                dsn::utils::PegasusEnv(dsn::utils::FileDataType::kSensitive),
                rocksdb::Slice(value.data(), value.length()),
                bulk_load_info_path,
                /* should_sync */ true);
            ASSERT_TRUE(s.ok()) << s.ToString();
        }

        // Write file '.bulk_load_info.meta'.
        {
            dist::block_service::file_metadata fm;
            ASSERT_TRUE(utils::filesystem::file_size(
                bulk_load_info_path, dsn::utils::FileDataType::kSensitive, fm.size));
            ASSERT_EQ(ERR_OK, utils::filesystem::md5sum(bulk_load_info_path, fm.md5));
            std::string value = nlohmann::json(fm).dump();
            string bulk_load_info_meta_path =
                fmt::format("{}/cluster/{}/.bulk_load_info.meta", LOCAL_BULK_LOAD_ROOT, app_name_);
            auto s = rocksdb::WriteStringToFile(
                dsn::utils::PegasusEnv(dsn::utils::FileDataType::kSensitive),
                rocksdb::Slice(value),
                bulk_load_info_meta_path,
                /* should_sync */ true);
            ASSERT_TRUE(s.ok()) << s.ToString();
        }
    }

    error_code start_bulk_load(bool ingest_behind = false)
    {
        return ddl_client_->start_bulk_load(app_name_, CLUSTER, PROVIDER, BULK_LOAD, ingest_behind)
            .get_value()
            .err;
    }

    void remove_file(const string &file_path)
    {
        NO_FATALS(run_cmd_from_project_root("rm " + file_path));
    }

    void make_inconsistent_bulk_load_info()
    {
        // Write file 'bulk_load_info'.
        string bulk_load_info_path =
            fmt::format("{}/cluster/{}/bulk_load_info", LOCAL_BULK_LOAD_ROOT, app_name_);
        {
            bulk_load_info bli;
            bli.app_id = app_id_ + 1;
            bli.app_name = app_name_ + "wrong";
            bli.partition_count = partition_count_ * 2;
            blob value = dsn::json::json_forwarder<bulk_load_info>::encode(bli);
            auto s = rocksdb::WriteStringToFile(
                dsn::utils::PegasusEnv(dsn::utils::FileDataType::kSensitive),
                rocksdb::Slice(value.data(), value.length()),
                bulk_load_info_path,
                /* should_sync */ true);
            ASSERT_TRUE(s.ok()) << s.ToString();
        }

        // Write file '.bulk_load_info.meta'.
        {
            dist::block_service::file_metadata fm;
            ASSERT_TRUE(utils::filesystem::file_size(
                bulk_load_info_path, dsn::utils::FileDataType::kSensitive, fm.size));
            ASSERT_EQ(ERR_OK, utils::filesystem::md5sum(bulk_load_info_path, fm.md5));
            std::string value = nlohmann::json(fm).dump();
            string bulk_load_info_meta_path =
                fmt::format("{}/cluster/{}/.bulk_load_info.meta", LOCAL_BULK_LOAD_ROOT, app_name_);
            auto s = rocksdb::WriteStringToFile(
                dsn::utils::PegasusEnv(dsn::utils::FileDataType::kSensitive),
                rocksdb::Slice(value),
                bulk_load_info_meta_path,
                /* should_sync */ true);
            ASSERT_TRUE(s.ok()) << s.ToString();
        }
    }

    void update_allow_ingest_behind(const string &allow_ingest_behind)
    {
        ASSERT_EQ(
            ERR_OK,
            ddl_client_
                ->set_app_envs(app_name_, {ROCKSDB_ALLOW_INGEST_BEHIND}, {allow_ingest_behind})
                .get_value()
                .err);
        LOG_INFO("sleep 31s to wait app_envs update");
        std::this_thread::sleep_for(std::chrono::seconds(31));
    }

    bulk_load_status::type wait_bulk_load_finish(int64_t remain_seconds)
    {
        int64_t sleep_time = 5;
        error_code err = ERR_OK;

        bulk_load_status::type last_status = bulk_load_status::BLS_INVALID;
        // when bulk load end, err will be ERR_INVALID_STATE
        while (remain_seconds > 0 && err == ERR_OK) {
            sleep_time = sleep_time > remain_seconds ? remain_seconds : sleep_time;
            remain_seconds -= sleep_time;
            LOG_INFO("sleep {}s to query bulk status", sleep_time);
            std::this_thread::sleep_for(std::chrono::seconds(sleep_time));

            auto resp = ddl_client_->query_bulk_load(app_name_).get_value();
            err = resp.err;
            if (err == ERR_OK) {
                last_status = resp.app_status;
            }
        }
        return last_status;
    }

    void verify_bulk_load_data()
    {
        NO_FATALS(verify_data("hashkey", "sortkey"));
        NO_FATALS(verify_data(HASHKEY_PREFIX, SORTKEY_PREFIX));
    }

    void verify_data(const string &hashkey_prefix, const string &sortkey_prefix)
    {
        for (int i = 0; i < COUNT; ++i) {
            string hash_key = hashkey_prefix + std::to_string(i);
            LOG_INFO("Start to verify hashkey: {}", hash_key);
            for (int j = 0; j < COUNT; ++j) {
                string sort_key = sortkey_prefix + std::to_string(j);
                string act_value;
                ASSERT_EQ(PERR_OK, client_->get(hash_key, sort_key, act_value)) << hash_key << ","
                                                                                << sort_key;
                ASSERT_EQ(VALUE, act_value) << hash_key << "," << sort_key;
            }
        }
    }

    enum operation
    {
        GET,
        SET,
        DEL,
        NO_VALUE
    };
    void operate_data(bulk_load_test::operation op, const string &value, int count)
    {
        for (int i = 0; i < count; ++i) {
            string hash_key = HASHKEY_PREFIX + std::to_string(i);
            string sort_key = SORTKEY_PREFIX + std::to_string(i);
            switch (op) {
            case bulk_load_test::operation::GET: {
                string act_value;
                ASSERT_EQ(PERR_OK, client_->get(hash_key, sort_key, act_value));
                ASSERT_EQ(value, act_value);
            } break;
            case bulk_load_test::operation::DEL: {
                ASSERT_EQ(PERR_OK, client_->del(hash_key, sort_key));
            } break;
            case bulk_load_test::operation::SET: {
                ASSERT_EQ(PERR_OK, client_->set(hash_key, sort_key, value));
            } break;
            case bulk_load_test::operation::NO_VALUE: {
                string act_value;
                ASSERT_EQ(PERR_NOT_FOUND, client_->get(hash_key, sort_key, act_value));
            } break;
            default:
                ASSERT_TRUE(false);
                break;
            }
        }
    }

protected:
    string BULK_LOAD_LOCAL_APP_ROOT;
    const string SOURCE_FILES_ROOT =
        "src/test/function_test/bulk_load_test/pegasus-bulk-load-function-test-files";
    const string LOCAL_SERVICE_ROOT = "onebox/block_service/local_service";
    const string LOCAL_BULK_LOAD_ROOT = "onebox/block_service/local_service/bulk_load_root";
    const string BULK_LOAD = "bulk_load_root";
    const string CLUSTER = "cluster";
    const string PROVIDER = "local_service";
    const string HASHKEY_PREFIX = "hash";
    const string SORTKEY_PREFIX = "sort";
    const string VALUE = "newValue";
    const int32_t COUNT = 100;
};

// Test bulk load failed because `bulk_load_info` file is missing
TEST_F(bulk_load_test, bulk_load_test_failed1)
{
    NO_FATALS(remove_file(BULK_LOAD_LOCAL_APP_ROOT + "/bulk_load_info"));
    ASSERT_EQ(ERR_OBJECT_NOT_FOUND, start_bulk_load());
}

// Test bulk load failed because `bulk_load_info` file inconsistent with current app_info
TEST_F(bulk_load_test, bulk_load_test_failed2)
{
    NO_FATALS(make_inconsistent_bulk_load_info());
    ASSERT_EQ(ERR_INCONSISTENT_STATE, start_bulk_load());
}

// Test bulk load failed because partition[0] `bulk_load_metadata` file is missing
TEST_F(bulk_load_test, bulk_load_test_failed3)
{
    NO_FATALS(remove_file(BULK_LOAD_LOCAL_APP_ROOT + "/0/bulk_load_metadata"));
    if (ERR_OK != start_bulk_load()) {
        assert(false);
    }
    ASSERT_EQ(bulk_load_status::BLS_FAILED, wait_bulk_load_finish(300));
}

///
/// case1: bulk load succeed with data verfied
/// case2: bulk load data consistent:
///     - old data will be overrided by bulk load data
///     - get/set/del succeed after bulk load
///
TEST_F(bulk_load_test, bulk_load_tests)
{
    // write old data
    NO_FATALS(operate_data(operation::SET, "oldValue", 10));
    NO_FATALS(operate_data(operation::GET, "oldValue", 10));

    ASSERT_EQ(ERR_OK, start_bulk_load());
    if (bulk_load_status::BLS_SUCCEED != wait_bulk_load_finish(300)) {
        assert(false);
    }
    LOG_INFO("Start to verify data...");
    NO_FATALS(verify_bulk_load_data());

    // values have been overwritten by bulk_load_data
    LOG_INFO("Start to GET data...");
    NO_FATALS(operate_data(operation::GET, VALUE, 10));

    // write new data succeed after bulk load
    LOG_INFO("Start to SET data...");
    NO_FATALS(operate_data(operation::SET, "valueAfterBulkLoad", 20));
    LOG_INFO("Start to GET data...");
    NO_FATALS(operate_data(operation::GET, "valueAfterBulkLoad", 20));

    // delete data succeed after bulk load
    LOG_INFO("Start to DEL data...");
    NO_FATALS(operate_data(operation::DEL, "", 15));
    LOG_INFO("Start to NO_VALUE data...");
    NO_FATALS(operate_data(operation::NO_VALUE, "", 15));
}

///
/// case1: inconsistent ingest_behind
/// case2: bulk load(ingest_behind) succeed with data verfied
/// case3: bulk load data consistent:
///     - bulk load data will be overrided by old data
///     - get/set/del succeed after bulk load
///
TEST_F(bulk_load_test, bulk_load_ingest_behind_tests)
{
    NO_FATALS(update_allow_ingest_behind("false"));

    // app envs allow_ingest_behind = false, request ingest_behind = true
    ASSERT_EQ(ERR_INCONSISTENT_STATE, start_bulk_load(true));

    NO_FATALS(update_allow_ingest_behind("true"));

    // write old data
    NO_FATALS(operate_data(operation::SET, "oldValue", 10));
    NO_FATALS(operate_data(operation::GET, "oldValue", 10));

    ASSERT_EQ(ERR_OK, start_bulk_load(true));
    ASSERT_EQ(bulk_load_status::BLS_SUCCEED, wait_bulk_load_finish(300));

    // values have been overwritten by bulk_load_data
    LOG_INFO("Start to GET data...");
    NO_FATALS(operate_data(operation::GET, "oldValue", 10));
    LOG_INFO("Start to verify data...");
    NO_FATALS(verify_data("hashkey", "sortkey"));

    // write new data succeed after bulk load
    LOG_INFO("Start to SET data...");
    NO_FATALS(operate_data(operation::SET, "valueAfterBulkLoad", 20));
    LOG_INFO("Start to GET data...");
    NO_FATALS(operate_data(operation::GET, "valueAfterBulkLoad", 20));

    // delete data succeed after bulk load
    LOG_INFO("Start to DEL data...");
    NO_FATALS(operate_data(operation::DEL, "", 15));
    LOG_INFO("Start to NO_VALUE data...");
    NO_FATALS(operate_data(operation::NO_VALUE, "", 15));
}
