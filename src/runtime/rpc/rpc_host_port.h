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

#include "utils/errors.h"
#include "utils/synchronize.h"

namespace dsn {

struct host_port_hash;

// TODO(yingchun): unit tests
class host_port
{
public:
    host_port() = default;
    host_port(std::string host, uint16_t port);

    host_port(const host_port &other);

    // Parse a <host>:<port> pair into this object.
    //
    // Note that <host> cannot be in IPv6 address notation.
    error_s parse_string(const std::string &str);

    void reset() {
        _host.clear();
        _port = 0;
    }

    bool initialized() const { return !_host.empty(); }

    const std::string &host() const { return _host; }
    uint16_t port() const { return _port; }

    std::string to_string() const { return fmt::format("{}:{}", _host, _port); }

    // Parse a comma separated list of "host:port" pairs into a vector
    // host_port objects.
    static error_s parse_strings(const std::string &comma_sep_addrs, std::vector<host_port> *res);

    static error_s
    load_servers(const std::string &section, const std::string &key, std::vector<host_port> *hps);

    host_port& operator=(const host_port& other)
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
    friend struct host_port_hash;

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

inline bool operator!=(const host_port &hp1, const host_port &hp2)
{
    return !(hp1 == hp2);
}

class host_port_group
{
public:
    host_port_group() = default;
    void add(const host_port& hp);
    host_port next(const host_port& hp) const;

    void set_rand_leader();
    void set_leader(const host_port& hp);
    host_port leader() const;

    std::string to_string() const;

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
    size_t operator()(const ::dsn::host_port& hp) const {
        return std::hash<std::string>()(hp.host()) ^ hp.port();
    }
};
} // namespace std
