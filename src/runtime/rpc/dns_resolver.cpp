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

#include "runtime/rpc/dns_resolver.h"

using std::vector;

namespace dsn {

rpc_address dns_resolver::resolve_address(const host_port &hp)
{
    if (!hp.initialized()) {
        return rpc_address();
    }
    vector<rpc_address> addresses;
    CHECK_OK(resolve_addresses(hp, &addresses), "host_port '{}' can not be resolved", hp);
    CHECK(!addresses.empty(), "host_port '{}' can not be resolved to any address", hp);
    if (addresses.size() > 1) {
        LOG_WARNING_F("host_port '{}' resolves to {} different addresses, using {}",
                      hp,
                      addresses.size(),
                      addresses[0]);
    }
    return addresses[0];
}

void dns_resolver::add_item_TEST(const host_port &hp, const rpc_address &addr)
{
    utils::auto_write_lock l(_lock);
    _cache_for_test.insert(std::make_pair(hp, addr));
}

error_s dns_resolver::resolve_addresses(const host_port &hp, vector<rpc_address> *addresses)
{
    if (get_cached_addresses(hp, addresses)) {
        return error_s::ok();
    }
    return do_resolution(hp, addresses);
}

bool dns_resolver::get_cached_addresses(const host_port &hp, vector<rpc_address> *addresses)
{
    CHECK_NOTNULL(addresses, "");
    utils::auto_read_lock l(_lock);
    const auto &found = _cache_for_test.find(hp);
    if (found == _cache_for_test.end()) {
        return false;
    }

    *addresses = {found->second};
    return true;
}

error_s dns_resolver::do_resolution(const host_port &hp, std::vector<rpc_address> *addresses)
{
    vector<rpc_address> resolved_addresses;
    RETURN_NOT_OK(hp.resolve_addresses(&resolved_addresses));

    // TODO: put to cache

    if (addresses) {
        *addresses = std::move(resolved_addresses);
    }

    return error_s::ok();
}

} // namespace dsn
