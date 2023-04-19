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

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "replica/replication_app_base.h"
#include "runtime/app_model.h"
#include "runtime/service_app.h"
#include "server/compaction_operation.h"
#include "server/pegasus_server_impl.h"
#include "utils/error_code.h"
#include "utils/utils.h"

std::atomic_bool gtest_done{false};
std::atomic_int gtest_ret{false};

using namespace pegasus;

class gtest_app : public service_app
{
public:
    explicit gtest_app(const service_app_info *info) : service_app(info) {}

    error_code start(const std::vector<std::string> &args) override
    {
        service_app::start(args);
        gtest_ret = RUN_ALL_TESTS();
        gtest_done = true;
        return ERR_OK;
    }
};

GTEST_API_ int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    service_app::register_factory<gtest_app>("replica");

    replication::replication_app_base::register_storage_engine(
        "pegasus", replication::replication_app_base::create<server::pegasus_server_impl>);
    server::register_compaction_operations();

    dsn_run_config("config.ini", false);
    while (!gtest_done) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    dsn_exit(gtest_ret);
}
