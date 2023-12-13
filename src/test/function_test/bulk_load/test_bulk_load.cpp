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

#include <fmt/core.h>
#include <rocksdb/env.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/sst_file_writer.h>
#include <rocksdb/status.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "base/pegasus_const.h"
#include "base/pegasus_key_schema.h"
#include "base/pegasus_utils.h"
#include "base/pegasus_value_schema.h"
#include "block_service/local/local_service.h"
#include "bulk_load_types.h"
#include "client/partition_resolver.h"
#include "client/replication_ddl_client.h"
#include "common/bulk_load_common.h"
#include "common/json_helper.h"
#include "gtest/gtest.h"
#include "meta/meta_bulk_load_service.h"
#include "metadata_types.h"
#include "test/function_test/utils/global_env.h"
#include "test/function_test/utils/test_util.h"
#include "utils/blob.h"
#include "utils/env.h"
#include "utils/error_code.h"
#include "utils/errors.h"
#include "utils/filesystem.h"
#include "utils/flags.h"
#include "utils/test_macros.h"
#include "utils/utils.h"

DSN_DECLARE_bool(encrypt_data_at_rest);

using namespace ::dsn;
using namespace ::dsn::replication;
using namespace pegasus;
using std::map;
using std::string;

class bulk_load_test : public test_util
{
protected:
    bulk_load_test() : test_util(map<string, string>({{"rocksdb.allow_ingest_behind", "true"}}))
    {
        TRICKY_CODE_TO_AVOID_LINK_ERROR;
        bulk_load_local_app_root_ = fmt::format("{}/{}/{}/{}/{}",
                                                global_env::instance()._pegasus_root,
                                                kLocalServiceRoot,
                                                kBulkLoad,
                                                kClusterName_,
                                                table_name_);
    }

    void SetUp() override
    {
        test_util::SetUp();
        NO_FATALS(generate_bulk_load_files());
    }

    void TearDown() override
    {
        ASSERT_EQ(ERR_OK, ddl_client_->drop_app(table_name_, 0));
        NO_FATALS(run_cmd_from_project_root("rm -rf " + bulk_load_local_app_root_));
    }

    // Generate the 'bulk_load_info' file according to 'bli' to path 'bulk_load_info_path'.
    void generate_bulk_load_info(const bulk_load_info &bli, const std::string &bulk_load_info_path)
    {
        auto value = dsn::json::json_forwarder<bulk_load_info>::encode(bli);
        auto s =
            rocksdb::WriteStringToFile(dsn::utils::PegasusEnv(dsn::utils::FileDataType::kSensitive),
                                       rocksdb::Slice(value.data(), value.length()),
                                       bulk_load_info_path,
                                       /* should_sync */ true);
        ASSERT_TRUE(s.ok()) << s.ToString();
    }

    // Generate the '.xxx.meta' file according to the 'xxx' file
    // in path 'file_path'.
    void generate_metadata_file(const std::string &file_path)
    {
        dist::block_service::file_metadata fm;
        ASSERT_TRUE(dsn::utils::filesystem::file_size(
            file_path, dsn::utils::FileDataType::kSensitive, fm.size));
        ASSERT_EQ(ERR_OK, dsn::utils::filesystem::md5sum(file_path, fm.md5));
        auto metadata_file_path = dist::block_service::local_service::get_metafile(file_path);
        ASSERT_EQ(ERR_OK, fm.write_to_file(metadata_file_path));
    }

    void generate_bulk_load_files()
    {
        // Generate data for each partition.
        for (int i = 0; i < partition_count_; i++) {
            // Prepare the path of partition i.
            auto partition_path = fmt::format("{}/{}", bulk_load_local_app_root_, i);
            NO_FATALS(run_cmd_from_project_root(fmt::format("mkdir -p {}", partition_path)));

            // Generate 2 sst files for each partition.
            for (int j = 0; j < 2; j++) {
                auto sst_file = fmt::format("{}/{:05}.sst", partition_path, j);

                // Create a SstFileWriter to generate the external sst file.
                rocksdb::EnvOptions env_opts;
                rocksdb::Options opts;
                rocksdb::SstFileWriter sfw(env_opts, opts);
                auto s = sfw.Open(sst_file);
                ASSERT_TRUE(s.ok()) << s.ToString();

                // Make sure the keys must be added in strict ascending order.
                //
                // TODO(yingchun): Now we write the same data to all partitions, i.e. hashkeys are
                //  <kHashKeyPrefix1><i>, it doesn't matter, the correct primary partition will be
                //  selected when read from client. That is to say, it will not take effect on the
                //  result.
                //  We can improve to generate only the data which belongs to the partition later.
                for (int k = 0; k < kBulkLoadItemCount; k++) {
                    dsn::blob bb_key;
                    pegasus_generate_key(
                        bb_key, fmt::format("{}{:04}", kHashKeyPrefix1, k), kDefaultSortkey);
                    std::string buf;
                    rocksdb::Slice value(pegasus_value_generator().generate_value(
                                             1, fmt::format("{}{}", kBulkLoadValue, k), 0, 0),
                                         &buf);
                    s = sfw.Put(pegasus::utils::to_rocksdb_slice(bb_key.to_string_view()), value);
                    ASSERT_TRUE(s.ok()) << s.ToString();
                }

                // Write 2 hashkey prefixes to check the functionality.
                for (int k = 0; k < kBulkLoadItemCount; k++) {
                    dsn::blob bb_key;
                    pegasus_generate_key(
                        bb_key, fmt::format("{}{:04}", kHashKeyPrefix2, k), kDefaultSortkey);
                    std::string buf;
                    rocksdb::Slice value(pegasus_value_generator().generate_value(
                                             1, fmt::format("{}{}", kBulkLoadValue, k), 0, 0),
                                         &buf);
                    s = sfw.Put(pegasus::utils::to_rocksdb_slice(bb_key.to_string_view()), value);
                    ASSERT_TRUE(s.ok()) << s.ToString();
                }
                s = sfw.Finish();
                ASSERT_TRUE(s.ok()) << s.ToString();
            }

            // Find the generated files.
            std::vector<std::string> src_files;
            ASSERT_TRUE(dsn::utils::filesystem::get_subfiles(
                partition_path, src_files, /* recursive */ false));
            ASSERT_FALSE(src_files.empty());

            bulk_load_metadata blm;
            for (const auto &src_file : src_files) {
                // Only .sst files are needed.
                if (src_file.find(".sst") == std::string::npos) {
                    continue;
                }

                // Get file name.
                file_meta fm;
                fm.name = dsn::utils::filesystem::get_file_name(src_file);

                // Get file size.
                int64_t file_size;
                ASSERT_TRUE(dsn::utils::filesystem::file_size(
                    src_file, dsn::utils::FileDataType::kSensitive, file_size));
                fm.size = file_size;
                blm.file_total_size += file_size;

                // Get file md5sum.
                ASSERT_EQ(dsn::ERR_OK, dsn::utils::filesystem::md5sum(src_file, fm.md5));

                // Write metadata for each file.
                NO_FATALS(generate_metadata_file(src_file));

                // Append the metadata.
                blm.files.push_back(fm);
            }

            // Generate 'bulk_load_metadata' file for each partition.
            blob bb = json::json_forwarder<bulk_load_metadata>::encode(blm);
            std::string blm_path = dsn::utils::filesystem::path_combine(
                partition_path, bulk_load_constant::BULK_LOAD_METADATA);
            auto s = rocksdb::WriteStringToFile(
                dsn::utils::PegasusEnv(dsn::utils::FileDataType::kSensitive),
                rocksdb::Slice(bb.data(), bb.length()),
                blm_path,
                /* should_sync */ true);
            ASSERT_TRUE(s.ok()) << s.ToString();

            // Generate '.bulk_load_metadata.meta' file of 'bulk_load_metadata'.
            NO_FATALS(generate_metadata_file(blm_path));
        }

        // Generate 'bulk_load_info' file for this table.
        auto bulk_load_info_path = fmt::format("{}/bulk_load_info", bulk_load_local_app_root_);
        NO_FATALS(generate_bulk_load_info(bulk_load_info(table_id_, table_name_, partition_count_),
                                          bulk_load_info_path));

        // Generate '.bulk_load_info.meta' file of 'bulk_load_info'.
        NO_FATALS(generate_metadata_file(bulk_load_info_path));

        // TODO(yingchun): not enabled yet!
        if (FLAGS_encrypt_data_at_rest) {
            std::vector<std::string> src_files;
            ASSERT_TRUE(dsn::utils::filesystem::get_subfiles(kLocalServiceRoot, src_files, true));
            for (const auto &src_file : src_files) {
                auto s = dsn::utils::encrypt_file(src_file);
                ASSERT_TRUE(s.ok()) << s.ToString();
            }
        }
    }

    error_code start_bulk_load(bool ingest_behind)
    {
        return ddl_client_
            ->start_bulk_load(table_name_, kClusterName_, "local_service", kBulkLoad, ingest_behind)
            .get_value()
            .err;
    }

    void remove_file(const string &file_path)
    {
        NO_FATALS(run_cmd_from_project_root("rm " + file_path));
    }

    bulk_load_status::type wait_bulk_load_finish(int64_t remain_seconds)
    {
        int64_t sleep_time = 5;
        auto err = ERR_OK;

        auto last_status = bulk_load_status::BLS_INVALID;
        // when bulk load end, err will be ERR_INVALID_STATE
        while (remain_seconds > 0 && err == ERR_OK) {
            sleep_time = std::min(sleep_time, remain_seconds);
            remain_seconds -= sleep_time;
            fmt::print("sleep {}s to query bulk status\n", sleep_time);
            std::this_thread::sleep_for(std::chrono::seconds(sleep_time));

            auto resp = ddl_client_->query_bulk_load(table_name_).get_value();
            err = resp.err;
            if (err == ERR_OK) {
                last_status = resp.app_status;
            }
        }
        return last_status;
    }

    void check_bulk_load(bool ingest_behind)
    {
        const std::string &kValueBeforeBulkLoad = "valueBeforeBulkLoad";
        const std::string &kValueAfterBulkLoad = "valueAfterBulkLoad";

        // 1. Write some data before bulk load.
        // <kHashKeyPrefix2>${i} : kDefaultSortkey -> <kValueBeforeBulkLoad>${i}, i=[0, 10]
        NO_FATALS(write_data(kHashKeyPrefix2, kValueBeforeBulkLoad, 10));
        NO_FATALS(verify_data(table_name_, kHashKeyPrefix2, kValueBeforeBulkLoad, 10));

        // 2. Start bulk load and wait until it complete.
        // <kHashKeyPrefix1>${i} : kDefaultSortkey -> <kBulkLoadValue>${i}, i=[0,
        // kBulkLoadItemCount]
        // <kHashKeyPrefix2>${i} : kDefaultSortkey -> <kBulkLoadValue>${i}, i=[0,
        // kBulkLoadItemCount]
        ASSERT_EQ(ERR_OK, start_bulk_load(ingest_behind));
        ASSERT_EQ(bulk_load_status::BLS_SUCCEED, wait_bulk_load_finish(300));

        // 3. Verify the data.
        fmt::print("Start to verify data...\n");
        if (ingest_behind) {
            // 3.a. Values have NOT been overwritten by the bulk load data.
            // i.  <kHashKeyPrefix2>${i} : kDefaultSortkey -> <kValueBeforeBulkLoad>${i}, i=[0, 10]
            NO_FATALS(verify_data(table_name_, kHashKeyPrefix2, kValueBeforeBulkLoad, 10));
            // ii. <kHashKeyPrefix1>${j} : kDefaultSortkey -> <kBulkLoadValue>${i}, j=[0,
            // kBulkLoadItemCount]
            NO_FATALS(
                verify_data(table_name_, kHashKeyPrefix1, kBulkLoadValue, kBulkLoadItemCount));
        } else {
            // 3.b. Values have been overwritten by the bulk load data.
            // i.  <kHashKeyPrefix1>${j} : kDefaultSortkey -> <kBulkLoadValue>${i}, j=[0,
            // kBulkLoadItemCount]
            NO_FATALS(
                verify_data(table_name_, kHashKeyPrefix1, kBulkLoadValue, kBulkLoadItemCount));
            // ii. <kHashKeyPrefix2>${i} : kDefaultSortkey -> <kBulkLoadValue>${i}, i=[0,
            // kBulkLoadItemCount]
            NO_FATALS(
                verify_data(table_name_, kHashKeyPrefix2, kBulkLoadValue, kBulkLoadItemCount));
        }

        // 4. Write new data after bulk load.
        // <kHashKeyPrefix2>${i} : kDefaultSortkey -> <kValueAfterBulkLoad>${i}, i=[0, 20]
        NO_FATALS(write_data(kHashKeyPrefix2, kValueAfterBulkLoad, 20));
        NO_FATALS(verify_data(table_name_, kHashKeyPrefix2, kValueAfterBulkLoad, 20));

        // 5. Delete data after bulk load.
        // <kHashKeyPrefix2>${i} : kDefaultSortkey -> none, i=[0, 15]
        NO_FATALS(delete_data(table_name_, kHashKeyPrefix2, 15));
        NO_FATALS(check_not_found(table_name_, kHashKeyPrefix2, 15));
    }

protected:
    string bulk_load_local_app_root_;
    const string kLocalServiceRoot = "onebox/block_service/local_service";
    const string kBulkLoad = "bulk_load_root";

    const int32_t kBulkLoadItemCount = 1000;
    const string kHashKeyPrefix1 = "hashkey1";
    const string kHashKeyPrefix2 = "hashkey2";
    const string kBulkLoadValue = "bulkLoadValue";
};

// Test bulk load failed because the 'bulk_load_info' file is missing
TEST_F(bulk_load_test, missing_bulk_load_info)
{
    NO_FATALS(remove_file(bulk_load_local_app_root_ + "/bulk_load_info"));
    ASSERT_EQ(ERR_OBJECT_NOT_FOUND, start_bulk_load(/* ingest_behind */ false));
}

// Test bulk load failed because the 'bulk_load_info' file is inconsistent with the actual app info.
TEST_F(bulk_load_test, inconsistent_bulk_load_info)
{
    std::string pegasus_root = global_env::instance()._pegasus_root;
    // Only 'app_id' and 'partition_count' will be checked in Pegasus server, so just inject these
    // kind of inconsistencies.
    bulk_load_info tests[] = {{table_id_ + 1, table_name_, partition_count_},
                              {table_id_, table_name_, partition_count_ * 2}};
    for (const auto &test : tests) {
        // Generate inconsistent 'bulk_load_info'.
        auto bulk_load_info_path = fmt::format("{}/bulk_load_info", bulk_load_local_app_root_);
        NO_FATALS(generate_bulk_load_info(test, bulk_load_info_path));

        // Generate '.bulk_load_info.meta'.
        NO_FATALS(generate_metadata_file(bulk_load_info_path));

        ASSERT_EQ(ERR_INCONSISTENT_STATE, start_bulk_load(/* ingest_behind */ false))
            << test.app_id << "," << test.app_name << "," << test.partition_count;
    }
}

// Test bulk load failed because partition[0]'s 'bulk_load_metadata' file is missing.
TEST_F(bulk_load_test, missing_p0_bulk_load_metadata)
{
    NO_FATALS(remove_file(bulk_load_local_app_root_ + "/0/bulk_load_metadata"));
    ASSERT_EQ(ERR_OK, start_bulk_load(/* ingest_behind */ false));
    ASSERT_EQ(bulk_load_status::BLS_FAILED, wait_bulk_load_finish(300));
}

// Test bulk load failed because the allow_ingest_behind config is inconsistent.
TEST_F(bulk_load_test, allow_ingest_behind_inconsistent)
{
    NO_FATALS(update_table_env({ROCKSDB_ALLOW_INGEST_BEHIND}, {"false"}));
    ASSERT_EQ(ERR_INCONSISTENT_STATE, start_bulk_load(/* ingest_behind */ true));
}

// Test normal bulk load, old data will be overwritten by bulk load data.
TEST_F(bulk_load_test, normal) { check_bulk_load(/* ingest_behind */ false); }

// Test normal bulk load with allow_ingest_behind=true, old data will NOT be overwritten by bulk
// load data.
TEST_F(bulk_load_test, allow_ingest_behind)
{
    NO_FATALS(update_table_env({ROCKSDB_ALLOW_INGEST_BEHIND}, {"true"}));
    check_bulk_load(/* ingest_behind */ true);
}
