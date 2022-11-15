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

#pragma once

#include <vector>

#include "runtime/rpc/rpc_address.h"
#include "runtime/rpc/rpc_host_port.h"
#include "utils/errors.h"

namespace dsn {

class dns_resolver
{
public:
    dns_resolver() = default;

    rpc_address resolve_address(const host_port &hp);

    void add_item_TEST(const host_port &hp, const rpc_address &addr);

private:
    error_s resolve_addresses(const host_port &hp, std::vector<rpc_address> *addresses);

    bool get_cached_addresses(const host_port &hp, std::vector<rpc_address> *addresses);

    error_s do_resolution(const host_port &hp, std::vector<rpc_address> *addresses);

    // TODO: add a ttl cache
    mutable utils::rw_lock_nr _lock;
    std::unordered_map<host_port, rpc_address> _cache_for_test;
};

} // namespace dsn
