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

#include <fmt/core.h>
#include <fmt/format.h>
#include <pegasus/client.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>
#include <unordered_set>
#include <utility>

#include "client/replication_ddl_client.h"
#include "common/common.h"
#include "common/gpid.h"
#include "common/replication_other_types.h"
#include "kill_testor.h"
#include "utils/error_code.h"
#include "utils/flags.h"
#include "utils/fmt_logging.h"

DSN_DEFINE_uint32(pegasus.killtest, kill_interval_seconds, 30, "");
DSN_DEFINE_uint32(pegasus.killtest, max_seconds_for_all_partitions_to_recover, 600, "");

DSN_DECLARE_string(pegasus_cluster_name);
DSN_DECLARE_string(verify_app_name);

namespace pegasus {
namespace test {

kill_testor::kill_testor(const char *config_file)
{
    // initialize the _client.
    if (!pegasus_client_factory::initialize(config_file)) {
        exit(-1);
    }

    // load meta_list
    dsn::replication::replica_helper::load_servers_from_config(
        dsn::PEGASUS_CLUSTER_SECTION_NAME, FLAGS_pegasus_cluster_name, meta_list);
    if (meta_list.empty()) {
        LOG_ERROR("Should config the meta address for killer");
        exit(-1);
    }

    ddl_client.reset(new replication_ddl_client(meta_list));
    if (!ddl_client) {
        LOG_ERROR("Initialize the _ddl_client failed");
        exit(-1);
    }

    srand((unsigned)time(nullptr));
}

kill_testor::~kill_testor() {}

void kill_testor::generate_random(std::vector<int> &res, int cnt, int a, int b)
{
    res.clear();
    if (a > b)
        std::swap(a, b);
    cnt = std::min(cnt, b - a + 1);
    std::unordered_set<int> numbers;
    int tvalue;
    for (int i = 0; i < cnt; i++) {
        tvalue = (rand() % (b - a + 1)) + a;
        while (numbers.find(tvalue) != numbers.end()) {
            tvalue = (rand() % (b - a + 1)) + a;
        }
        numbers.insert(tvalue);
        res.emplace_back(tvalue);
    }
}

int kill_testor::generate_one_number(int a, int b)
{
    if (a > b)
        std::swap(a, b);
    return ((rand() % (b - a + 1)) + a);
}

dsn::error_code kill_testor::get_partition_info(bool debug_unhealthy,
                                                int &healthy_partition_cnt,
                                                int &unhealthy_partition_cnt)
{
    healthy_partition_cnt = 0, unhealthy_partition_cnt = 0;
    int32_t app_id;
    int32_t partition_count;
    pcs.clear();
    dsn::error_code err = ddl_client->list_app(FLAGS_verify_app_name, app_id, partition_count, pcs);

    if (err == ::dsn::ERR_OK) {
        LOG_DEBUG("access meta and query partition status success");
        for (const auto &pc : pcs) {
            int replica_count = 0;
            dsn::host_port primary;
            GET_HOST_PORT(pc, primary, primary);
            if (primary) {
                replica_count++;
            }
            std::vector<dsn::host_port> secondaries;
            GET_HOST_PORTS(pc, secondaries, secondaries);
            replica_count += secondaries.size();
            if (replica_count == pc.max_replica_count) {
                healthy_partition_cnt++;
            } else {
                const auto &info =
                    fmt::format("gpid={}, primary={}, secondaries=[{}], last_committed_decree={}",
                                pc.pid,
                                primary,
                                fmt::join(secondaries, ", "),
                                pc.last_committed_decree);
                if (debug_unhealthy) {
                    LOG_INFO("found unhealthy partition, {}", info);
                } else {
                    LOG_DEBUG("found unhealthy partition, {}", info);
                }
            }
        }
        unhealthy_partition_cnt = partition_count - healthy_partition_cnt;
    } else {
        LOG_DEBUG("access meta and query partition status fail");
        healthy_partition_cnt = 0;
        unhealthy_partition_cnt = 0;
    }
    return err;
}

// false == partition unhealth, true == health
bool kill_testor::check_cluster_status()
{
    int healthy_partition_cnt = 0;
    int unhealthy_partition_cnt = 0;
    int try_count = 1;
    while (try_count <= FLAGS_max_seconds_for_all_partitions_to_recover) {
        dsn::error_code err =
            get_partition_info(try_count == FLAGS_max_seconds_for_all_partitions_to_recover,
                               healthy_partition_cnt,
                               unhealthy_partition_cnt);
        if (err == dsn::ERR_OK) {
            if (unhealthy_partition_cnt > 0) {
                LOG_DEBUG("query partition status success, but still have unhealthy partition, "
                          "healthy_partition_count = {}, unhealthy_partition_count = {}",
                          healthy_partition_cnt,
                          unhealthy_partition_cnt);
                sleep(1);
            } else
                return true;
        } else {
            LOG_INFO("query partition status fail, try times = {}", try_count);
            sleep(1);
        }
        try_count += 1;
    }

    return false;
}

} // namespace test
} // namespace pegasus
