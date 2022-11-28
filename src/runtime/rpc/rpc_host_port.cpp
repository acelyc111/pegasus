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

#include "runtime/rpc/rpc_host_port.h"

#include <boost/algorithm/string/trim.hpp>
#include <set>

#include "utils/config_api.h"
#include "utils/fmt_logging.h"
#include "utils/rand.h"
#include "utils/string_conv.h"
#include "utils/strings.h"

using std::set;
using std::string;
using std::vector;

namespace dsn {

host_port::host_port(string host, uint16_t port) : _host(std::move(host)), _port(port) {}

host_port::host_port(const host_port &other) : _host(other._host), _port(other._port) {}

error_s host_port::parse_string(const string &str)
{
    vector<string> hostname_port;
    utils::split_args(str.c_str(), hostname_port, ':');
    if (hostname_port.size() != 2) {
        return error_s::make(ERR_INVALID_PARAMETERS,
                             fmt::format("invalid host:port format: {}", str));
    }
    for (auto &v : hostname_port) {
        boost::trim(v);
    }

    // Parse the port.
    uint32_t port;
    if (!buf2uint32(hostname_port[1], port) || port > UINT16_MAX) {
        return error_s::make(ERR_INVALID_PARAMETERS,
                             fmt::format("invalid port: {}", hostname_port[1]));
    }

    _host.swap(hostname_port[0]);
    _port = port;
    return error_s::ok();
}

error_s host_port::parse_strings(const string &comma_sep_addrs, vector<host_port> *hps)
{
    CHECK_NOTNULL(hps, "");
    hps->clear();

    vector<string> addrs;
    utils::split_args(comma_sep_addrs.c_str(), addrs, ',');

    // TODO(yingchun): simply use to_string() for host_port set, we can override more operators if
    // needed in the future.
    set<string> hp_set;
    hps->reserve(addrs.size());
    for (const string &addr : addrs) {
        host_port hp;
        RETURN_NOT_OK(hp.parse_string(addr));
        if (hp_set.count(hp.to_string()) != 0) {
            LOG_WARNING_F("duplicate host:port '{}'", addr);
            continue;
        }
        hp_set.emplace(std::move(hp.to_string()));
        hps->emplace_back(std::move(hp));
    }
    return error_s::ok();
}

error_s host_port::load_servers(const string &section, const string &key, vector<host_port> *hps)
{
    CHECK_NOTNULL(hps, "");
    hps->clear();

    string comma_sep_addrs = dsn_config_get_value_string(section.c_str(), key.c_str(), "", "");
    return parse_strings(comma_sep_addrs, hps);
}

bool remove_node(const host_port& node, /*inout*/ vector<host_port> &node_list)
{
    auto it = std::find(node_list.begin(), node_list.end(), node);
    if (it != node_list.end()) {
        node_list.erase(it);
        return true;
    } else {
        return false;
    }
}

host_port_group::host_port_group(const host_port_group& other)
{
    utils::auto_read_lock l(other._lock);
    _members = other._members;
    _leader_index = other._leader_index;
}

void host_port_group::add(const host_port& hp)
{
    CHECK(hp.initialized(), "host_port is not initialized");
    utils::auto_write_lock l(_lock);
    if (_members.end() != std::find(_members.begin(), _members.end(), hp)) {
        LOG_WARNING_F("duplicate host_port '{}' will be ignored", hp);
        return;
    }
    _members.push_back(hp);
}

void host_port_group::add_list(const std::vector<host_port>& hps)
{
    for (const auto& hp : hps) {
        add(hp);
    }
}

host_port host_port_group::next(const host_port& hp) const
{
    utils::auto_read_lock l(_lock);
    if (_members.empty()) {
        return host_port();
    }

    // TODO(yingchun): the following code can be optimized
    if (!hp.initialized()) {
        return _members[rand::next_u32(0, _members.size() - 1)];
    }

    auto it = std::find(_members.begin(), _members.end(), hp);
    if (it == _members.end()) {
        return _members[rand::next_u32(0, _members.size() - 1)];
    }

    it++;
    return it == _members.end() ? _members[0] : *it;
}

void host_port_group::set_leader(const host_port& hp)
{
    utils::auto_write_lock l(_lock);
    if (!hp.initialized()) {
        _leader_index = -1;
        return;
    }

    for (int i = 0; i < _members.size(); ++i) {
        if (_members[i] == hp) {
            _leader_index = i;
            return;
        }
    }

    // TODO(yingchun): consider to remove push_back
    _members.push_back(hp);
    _leader_index = static_cast<int>(_members.size() - 1);
}

void host_port_group::set_rand_leader()
{
    utils::auto_write_lock l(_lock);
    _leader_index = rand::next_u32(0, static_cast<uint32_t>(_members.size()) - 1);
}

host_port host_port_group::leader() const
{
    utils::auto_read_lock l(_lock);
    return _leader_index >= 0 ? _members[_leader_index] : host_port();
}

std::string host_port_group::to_string() const
{
    utils::auto_read_lock l(_lock);
    return fmt::format("{}", fmt::join(_members, ","));
}

} // namespace dsn
