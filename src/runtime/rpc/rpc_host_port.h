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

#include <string>
#include <vector>

#include "runtime/rpc/rpc_address.h"
#include "utils/errors.h"
#include "utils/synchronize.h"

namespace dsn {

class rpc_address;
// TODO(yingchun): unit tests
class host_port
{
public:
    host_port() = default;
    host_port(std::string host, uint16_t port);
    // TODO: implemant me
    host_port(rpc_address) {}

    // TODO: for test
    host_port(int ip, uint16_t port) {}

    host_port(const host_port &other);

    // Parse a <host>:<port> pair into this object.
    //
    // Note that <host> cannot be in IPv6 address notation.
    error_s parse_string(const std::string &str);

    error_s parse_rpc_address(rpc_address addr);

    void reset()
    {
        _host.clear();
        _port = 0;
    }

    bool initialized() const { return !_host.empty(); }
    // TODO(yingchun): change
    bool is_invalid() const { return !initialized(); }

    error_s resolve_addresses(std::vector<rpc_address> *addresses) const;

    const std::string &host() const { return _host; }
    uint16_t port() const { return _port; }

    std::string to_string() const { return fmt::format("{}:{}", _host, _port); }

    host_port &operator=(const host_port &other)
    {
        _host = other._host;
        _port = other._port;
        return *this;
    }

    friend std::ostream &operator<<(std::ostream &os, const host_port &hp)
    {
        return os << hp.to_string();
    }

    // for serialization in thrift format
    uint32_t read(::apache::thrift::protocol::TProtocol *iprot);
    uint32_t write(::apache::thrift::protocol::TProtocol *oprot) const;

private:
    std::string _host;
    uint16_t _port = 0;
};

inline bool operator<(const host_port &hp1, const host_port &hp2)
{
    if (hp1.host() == hp2.host()) {
        return hp1.port() < hp2.port();
    }

    return hp1.host() < hp2.host();
}

inline bool operator==(const host_port &hp1, const host_port &hp2)
{
    return hp1.port() == hp2.port() && hp1.host() == hp2.host();
}

inline bool operator!=(const host_port &hp1, const host_port &hp2) { return !(hp1 == hp2); }

bool remove_node(const host_port &node,
                 /*inout*/ std::vector<host_port> &node_list); // TODO(yingchun): WARN_UNUSED_RESULT

class host_port_group
{
public:
    host_port_group() = default;
    host_port_group(const host_port_group &other);

    void add(const host_port &hp);
    void add_list(const std::vector<host_port> &hps);
    host_port next(const host_port &hp) const;
    void set_rand_leader();
    void set_leader(const host_port &hp);
    host_port leader() const;
    void clear();
    bool empty() const;
    const std::vector<host_port> &members() const;

    std::string to_string() const;

    static error_s
    load_servers(const std::string &section, const std::string &key, host_port_group *hpg);

    inline host_port_group &operator=(const host_port_group &other)
    {
        utils::auto_write_lock l(_lock);
        _leader_index = other._leader_index;
        _members = other._members;
        return *this;
    }

    inline bool operator==(const host_port_group &other) const
    {
        // TODO: optimize
        utils::auto_read_lock l(_lock);
        utils::auto_read_lock r(other._lock);
        std::set<host_port> hps1(_members.begin(), _members.end());
        std::set<host_port> hps2(other._members.begin(), other._members.end());
        return hps1 == hps2;
    }

    inline bool operator!=(const host_port_group &other) const { return !(*this == other); }

    friend std::ostream &operator<<(std::ostream &os, const host_port_group &hpg)
    {
        return os << hpg.to_string();
    }

private:
    mutable utils::rw_lock_nr _lock;
    int _leader_index;
    std::vector<host_port> _members;
};

} // namespace dsn

namespace std {
template <>
struct hash<::dsn::host_port>
{
    size_t operator()(const ::dsn::host_port &hp) const
    {
        return std::hash<std::string>()(hp.host()) ^ hp.port();
    }
};
} // namespace std
