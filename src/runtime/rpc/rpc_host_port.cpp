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

#include "utils/fmt_logging.h"

using std::string;
using std::vector;

namespace dsn {

host_port::host_port(string host, uint16_t port)
    : _host(std::move(host)), _port(port) {}

error_s host_port::parse_string(const string& str) {
    vector<string> hostname_port;
    utils::split_args(str.c_str(), hostname_port, ':');
    if (hostname_port.size() != 2) {
        return error_s::make(ERR_INVALID_PARAMETERS, fmt::format("invalid host:port format: {}", str));
    }
    for (auto& v : hostname_port) {
        boost::trim(v);
    }

    // Parse the port.
    uint32_t port;
    if (!buf2uint32(hostname_port[1], port) || port > UINT16_MAX) {
        return error_s::make(ERR_INVALID_PARAMETERS, fmt::format("invalid port: {}", hostname_port[1]));
    }

    host_.swap(hostname_port[0]);
    port_ = port;
    return error_s::ok();
}

error_s host_port::parse_strings(const string& comma_sep_addrs, vector<host_port>* hps)
{
    CHECK_NOTNULL(hps);
    hps->clear();

    vector<string> addrs;
    utils::split_args(comma_sep_addrs.c_str(), addrs, ',');

    set<host_port> hp_set;
    hps->reserve(addrs.size());
    for (const string& addr : addrs) {
        host_port hp;
        RETURN_NOT_OK(hp.parse_string(addr));
        if (hp_set.count(hp) != 0) {
            LOG_WARNING_F("duplicate host:port '{}'", addr);
            continue;
        }
        hps->emplace_back(std::move(hp));
    }
    return error_s::ok();
}

error_s host_port::load_servers(const string &section,
                                const string &key,
                                vector<host_port>* hps)
{
    CHECK_NOTNULL(hps);
    hps->clear();

    string comma_sep_addrs = dsn_config_get_value_string(section.c_str(), key.c_str(), "", "");
    return parse_strings(comma_sep_addrs, hps);
}

} // namespace dsn