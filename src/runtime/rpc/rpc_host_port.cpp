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

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <functional>
#include <set>
#include <unordered_set>

#include <boost/algorithm/string/trim.hpp>

#include "utils/config_api.h"
#include "utils/fmt_logging.h"
#include "utils/rand.h"
#include "utils/safe_strerror_posix.h"
#include "utils/string_conv.h"
#include "utils/strings.h"

using std::function;
using std::set;
using std::string;
using std::unique_ptr;
using std::unordered_set;
using std::vector;

// Mac OS 10.9 does not appear to define HOST_NAME_MAX in unistd.h
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif

namespace dsn {

namespace {

// Parse a comma separated list of "host:port" pairs into a vector host_port objects.
error_s parse_strings(const string &comma_sep_addrs, vector<host_port> *hps)
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
        hp_set.emplace(hp.to_string());
        hps->emplace_back(std::move(hp));
    }

    return error_s::ok();
}

using AddrInfo = unique_ptr<addrinfo, function<void(addrinfo *)>>;

// TODO: test, copy from Kudu
error_s GetAddrInfo(const string &hostname, const addrinfo &hints, AddrInfo *info)
{
    addrinfo *res = nullptr;
    const int rc = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);
    const int err = errno; // preserving the errno from the getaddrinfo() call
    AddrInfo result(res, ::freeaddrinfo);
    if (rc != 0) {
        if (rc == EAI_SYSTEM) {
            return error_s::make(ERR_NETWORK_FAILURE, utils::safe_strerror(err));
        }
        return error_s::make(ERR_NETWORK_FAILURE, gai_strerror(rc));
    }

    if (info != nullptr) {
        info->swap(result);
    }

    return error_s::ok();
}

// TODO(yingchun): duplicated, should refactor
error_s hostname_from_ip(uint32_t ip, string *hostname_result)
{
    CHECK_NOTNULL(hostname_result, "");
    struct sockaddr_in addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = 0;
    addr_in.sin_addr.s_addr = ip;
    char hostname[256];
    int rc = getnameinfo((struct sockaddr *)(&addr_in),
                         sizeof(struct sockaddr),
                         hostname,
                         sizeof(hostname),
                         nullptr,
                         0,
                         NI_NAMEREQD);
    const int err = errno; // preserving the errno from the getaddrinfo() call
    if (rc != 0) {
        struct in_addr net_addr;
        net_addr.s_addr = ip;
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &net_addr, ip_str, sizeof(ip_str));
        if (rc == EAI_SYSTEM) {
            return error_s::make(ERR_NETWORK_FAILURE, utils::safe_strerror(err));
        }
        return error_s::make(ERR_NETWORK_FAILURE, gai_strerror(rc));
    }
    *hostname_result = string(hostname);
    return error_s::ok();
}

} // anonymous namespace

host_port::host_port(string host, uint16_t port) : _host(std::move(host)), _port(port) {}

host_port::host_port(const host_port &other) : _host(other._host), _port(other._port) {}

host_port::host_port(rpc_address addr) { CHECK(parse_string(addr.to_std_string()).is_ok(), ""); }

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
    if (hostname_port[0].empty() || !buf2uint32(hostname_port[1], port) || port > UINT16_MAX) {
        return error_s::make(
            ERR_INVALID_PARAMETERS,
            fmt::format("invalid host:port({}:{})", hostname_port[0], hostname_port[1]));
    }

    _host.swap(hostname_port[0]);
    _port = static_cast<uint16_t>(port);
    return error_s::ok();
}

error_s host_port::parse_rpc_address(rpc_address addr)
{
    if (addr.is_invalid()) {
        return error_s::make(ERR_INVALID_PARAMETERS, fmt::format("invalid rpc_address({})", addr));
    }

    RETURN_NOT_OK(hostname_from_ip(htonl(addr.ip()), &_host));
    _port = addr.port();
    return error_s::ok();
}

error_s host_port::resolve_addresses(vector<rpc_address> *addresses) const
{
    // TODO: trace latency
    rpc_address rpc_addr;
    if (rpc_addr.from_string_ipv4(this->to_string().c_str())) {
        if (addresses) {
            addresses->push_back(rpc_addr);
        }
        return error_s::ok();
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    AddrInfo result;
    RETURN_NOT_OK(GetAddrInfo(_host, hints, &result));

    // DNS may return the same host multiple times. We want to return only the unique
    // addresses, but in the same order as DNS returned them. To do so, we keep track
    // of the already-inserted elements in a set.
    unordered_set<rpc_address> inserted;
    vector<rpc_address> result_addresses;
    for (const addrinfo *ai = result.get(); ai != nullptr; ai = ai->ai_next) {
        CHECK_EQ(AF_INET, ai->ai_family);
        sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(ai->ai_addr);
        addr->sin_port = htons(_port);
        rpc_address rpc_addr(*addr);
        LOG_INFO_F("resolved address {} for host_port {}", rpc_addr, to_string());
        if (inserted.insert(rpc_addr).second) {
            result_addresses.emplace_back(rpc_addr);
        }
    }
    if (addresses) {
        *addresses = std::move(result_addresses);
    }
    return error_s::ok();
}

bool remove_node(const host_port &node, /*inout*/ vector<host_port> &node_list)
{
    auto it = std::find(node_list.begin(), node_list.end(), node);
    if (it != node_list.end()) {
        node_list.erase(it);
        return true;
    } else {
        return false;
    }
}

host_port_group::host_port_group(const host_port_group &other)
{
    utils::auto_read_lock l(other._lock);
    _members = other._members;
    _leader_index = other._leader_index;
}

void host_port_group::add(const host_port &hp)
{
    CHECK(hp.initialized(), "host_port is not initialized");
    utils::auto_write_lock l(_lock);
    if (_members.end() != std::find(_members.begin(), _members.end(), hp)) {
        LOG_WARNING_F("duplicate host_port '{}' will be ignored", hp);
        return;
    }
    _members.push_back(hp);
    if (_members.size() == 1) {
        _leader_index = 0;
    }
}

void host_port_group::add_list(const std::vector<host_port> &hps)
{
    for (const auto &hp : hps) {
        add(hp);
    }
}

host_port host_port_group::next(const host_port &hp) const
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

void host_port_group::set_leader(const host_port &hp)
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
    CHECK_GE(_leader_index, 0);
    CHECK(!_members.empty(), "");
    return _members[_leader_index];
}

void host_port_group::clear()
{
    utils::auto_write_lock l(_lock);
    _leader_index = -1;
    _members.clear();
}

bool host_port_group::empty() const
{
    utils::auto_read_lock l(_lock);
    return _members.empty();
}
const std::vector<host_port> &host_port_group::members() const
{
    utils::auto_read_lock l(_lock);
    return _members;
}

string host_port_group::to_string() const
{
    utils::auto_read_lock l(_lock);
    return fmt::format("{}", fmt::join(_members, ","));
}

error_s
host_port_group::load_servers(const string &section, const string &key, host_port_group *hpg)
{
    CHECK_NOTNULL(hpg, "");
    hpg->clear();

    string comma_sep_addrs = dsn_config_get_value_string(section.c_str(), key.c_str(), "", "");

    vector<host_port> hps;
    RETURN_NOT_OK(parse_strings(comma_sep_addrs, &hps));

    hpg->add_list(hps);
    if (hpg->empty()) {
        return error_s::make(ERR_INVALID_PARAMETERS,
                             fmt::format("config {}.{} should not be empty", section, key));
    }

    return error_s::ok();
}

} // namespace dsn
