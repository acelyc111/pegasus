/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
 */

#include "test_util.h"

#include <nlohmann/json.hpp>
#include <pegasus/error.h>
#include <stdio.h>
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <initializer_list>
#include <thread>
#include <utility>
#include <vector>

#include "base/pegasus_const.h"
#include "client/replication_ddl_client.h"
#include "common/replication_other_types.h"
#include "fmt/core.h"
#include "gtest/gtest.h"
#include "include/pegasus/client.h"
#include "meta_admin_types.h"
#include "nlohmann/detail/iterators/iter_impl.hpp"
#include "nlohmann/json_fwd.hpp"
#include "runtime/api_layer1.h"
#include "runtime/rpc/rpc_address.h"
#include "test/function_test/utils/global_env.h"
#include "test/function_test/utils/utils.h"
#include "test_util/test_util.h"
#include "utils/defer.h"
#include "utils/error_code.h"
#include "utils/errors.h"
#include "utils/filesystem.h"
#include "utils/rand.h"

using dsn::partition_configuration;
using dsn::replication::replica_helper;
using dsn::replication::replication_ddl_client;
using dsn::rpc_address;
using nlohmann::json;
using std::map;
using std::string;
using std::vector;

namespace pegasus {

test_util::test_util(map<string, string> create_envs)
    : cluster_name_("onebox"), table_name_("temp"), create_envs_(std::move(create_envs))
{
}

test_util::~test_util() {}

void test_util::SetUpTestCase() { ASSERT_TRUE(pegasus_client_factory::initialize("config.ini")); }

void test_util::SetUp()
{
    ASSERT_TRUE(replica_helper::load_meta_servers(
        meta_list_, PEGASUS_CLUSTER_SECTION_NAME.c_str(), cluster_name_.c_str()));
    ASSERT_FALSE(meta_list_.empty());

    ddl_client_ = std::make_shared<replication_ddl_client>(meta_list_);
    ASSERT_TRUE(ddl_client_ != nullptr);
    ddl_client_->set_max_wait_app_ready_secs(120);

    dsn::error_code ret =
        ddl_client_->create_app(table_name_, "pegasus", partition_count_, 3, create_envs_, false);
    if (ret == dsn::ERR_INVALID_PARAMETERS) {
        ASSERT_EQ(dsn::ERR_OK, ddl_client_->drop_app(table_name_, 0));
        ASSERT_EQ(dsn::ERR_OK,
                  ddl_client_->create_app(
                      table_name_, "pegasus", partition_count_, 3, create_envs_, false));
    } else {
        ASSERT_EQ(dsn::ERR_OK, ret);
    }
    client_ = pegasus_client_factory::get_client(cluster_name_.c_str(), table_name_.c_str());
    ASSERT_TRUE(client_ != nullptr);

    int32_t partition_count;
    ASSERT_EQ(dsn::ERR_OK,
              ddl_client_->list_app(table_name_, table_id_, partition_count, partitions_));
    ASSERT_NE(0, table_id_);
    ASSERT_EQ(partition_count_, partition_count);
    ASSERT_EQ(partition_count_, partitions_.size());
}

void test_util::run_cmd_from_project_root(const string &cmd)
{
    ASSERT_EQ(0, ::chdir(global_env::instance()._pegasus_root.c_str()));
    ASSERT_NO_FATAL_FAILURE(run_cmd_no_error(cmd));
}

int test_util::get_alive_replica_server_count()
{
    const auto json_filename = fmt::format("test_json_file.{}", dsn::rand::next_u32());
    auto cleanup =
        dsn::defer([json_filename]() { dsn::utils::filesystem::remove_path(json_filename); });
    run_cmd_from_project_root(fmt::format("echo 'nodes -djo {}' | ./run.sh shell", json_filename));
    std::ifstream f(json_filename);
    const auto data = json::parse(f);
    vector<string> rs_addrs;
    for (const auto &rs : data["details"]) {
        if (rs["status"] == "UNALIVE") {
            continue;
        }
        rs_addrs.push_back(rs["address"]);
    }

    int replica_server_count = 0;
    for (const auto &rs_addr : rs_addrs) {
        int ret = run_cmd(fmt::format("curl {}/version", rs_addr));
        if (ret == 0) {
            replica_server_count++;
        }
    }
    return replica_server_count;
}

int test_util::get_leader_count(const string &table_name, int replica_server_index)
{
    const auto json_filename = fmt::format("test_json_file.{}", dsn::rand::next_u32());
    auto cleanup =
        dsn::defer([json_filename]() { dsn::utils::filesystem::remove_path(json_filename); });
    run_cmd_from_project_root(
        fmt::format("echo 'app {} -djo {}' | ./run.sh shell", table_name, json_filename));
    std::ifstream f(json_filename);
    const auto data = json::parse(f);
    int leader_count = 0;
    for (const auto &replica : data["replicas"]) {
        const auto &primary = to_string(replica["primary"]);
        if (primary.find(fmt::format("3480{}", replica_server_index)) != string::npos) {
            leader_count++;
        }
    }
    return leader_count;
}

void test_util::wait_table_healthy(const std::string &table_name) const
{
    ASSERT_IN_TIME(
        [&] {
            int32_t table_id = 0;
            int32_t pcount = 0;
            std::vector<partition_configuration> pcs;
            ASSERT_EQ(dsn::ERR_OK, ddl_client_->list_app(table_name, table_id, pcount, pcs));
            for (const auto &pc : pcs) {
                ASSERT_FALSE(pc.primary.is_invalid());
                ASSERT_EQ(1 + pc.secondaries.size(), pc.max_replica_count);
            }
        },
        180);
}

void test_util::write_data(int count) const
{
    fmt::print(stdout, "start to write {} key-value pairs...\n", count);
    ASSERT_NE(client_, nullptr);
    int64_t start = dsn_now_ms();
    for (int i = 0; i < count; i++) {
        ASSERT_EQ(PERR_OK,
                  client_->set(fmt::format("hash_key_{}", i),
                               fmt::format("sort_key_{}", i),
                               fmt::format("value_{}", i)));
    }
    fmt::print(stdout, "write data complete, total time = {}s", (dsn_now_ms() - start) / 1000);
}

void test_util::verify_data(const std::string &table_name, int count) const
{
    fmt::print(stdout, "start to get {} key-value pairs...\n", count);
    auto *client = pegasus_client_factory::get_client(cluster_name_.c_str(), table_name.c_str());
    ASSERT_NE(client, nullptr);
    int64_t start = dsn_now_ms();
    for (int i = 0; i < count; i++) {
        std::string value_new;
        ASSERT_EQ(
            PERR_OK,
            client->get(fmt::format("hash_key_{}", i), fmt::format("sort_key_{}", i), value_new));
        ASSERT_EQ(fmt::format("value_{}", i), value_new);
    }
    int64_t end = dsn_now_ms();
    fmt::print(stdout, "verify data complete, total time = {}s", (dsn_now_ms() - start) / 1000);
}

void test_util::update_table_env(const std::vector<std::string> &keys,
                                 const std::vector<std::string> &values) const
{
    auto resp = ddl_client_->set_app_envs(table_name_, keys, values);
    ASSERT_TRUE(resp.is_ok());
    ASSERT_EQ(dsn::ERR_OK, resp.get_value().err);
    // TODO(yingchun): update the sync interval to reduce time.
    fmt::print(stdout, "sleep 31s to wait app_envs update\n");
    std::this_thread::sleep_for(std::chrono::seconds(31));
}
} // namespace pegasus
