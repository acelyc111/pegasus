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

#pragma once

#include <gtest/gtest_prod.h>

#include "replica/replica.h"

namespace dsn {
namespace replication {

class replica_follower : replica_base
{
public:
    explicit replica_follower(replica *r);
    ~replica_follower();
    error_code duplicate_checkpoint();

    const std::string &get_master_cluster_name() const { return _master_cluster_name; };

    const std::string &get_master_app_name() const { return _master_app_name; };

    const bool is_need_duplicate() const { return need_duplicate; }

protected:
    replica *_replica = nullptr;

private:
    friend class replica_follower_test;
    FRIEND_TEST(replica_follower_test, test_init_master_info);

    const host_port_group &get_master_meta_list() const { return _master_meta_list; };

    task_tracker _tracker;
    bool _duplicating_checkpoint{false};
    mutable zlock _lock;

    std::string _master_cluster_name;
    std::string _master_app_name;
    host_port_group _master_meta_list;
    partition_configuration _master_replica_config;

    bool need_duplicate{false};

    void init_master_info();
    void async_duplicate_checkpoint_from_master_replica();
    error_code update_master_replica_config(error_code err, query_cfg_response &&resp);
    void copy_master_replica_checkpoint();
    error_code nfs_copy_checkpoint(error_code err, learn_response &&resp);
    void nfs_copy_remote_files(const host_port &remote_node,
                               const std::string &remote_disk,
                               const std::string &remote_dir,
                               std::vector<std::string> &file_list,
                               const std::string &dest_dir);

    std::string master_replica_name()
    {
        std::string app_info = fmt::format("{}.{}", _master_cluster_name, _master_app_name);
        if (_master_replica_config.host_port_primary.initialized()) {
            return fmt::format("{}({}|{})",
                               app_info,
                               _master_replica_config.host_port_primary,
                               _master_replica_config.pid);
        }
        return app_info;
    }
};

} // namespace replication
} // namespace dsn
