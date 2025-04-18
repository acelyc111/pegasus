/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Microsoft Corporation
 *
 * -=- Robust Distributed System Nucleus (rDSN) -=-
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef replication_OTHER_TYPES_H
#define replication_OTHER_TYPES_H

#include <sstream>
#include "meta_admin_types.h"
#include "partition_split_types.h"
#include "duplication_types.h"
#include "bulk_load_types.h"
#include "backup_types.h"
#include "consensus_types.h"
#include "replica_admin_types.h"
#include "common/replication_enums.h"
#include "rpc/rpc_address.h"
#include "rpc/rpc_host_port.h"

namespace dsn {
namespace replication {

typedef int32_t app_id;
typedef int64_t ballot;
typedef int64_t decree;

#define invalid_ballot ((::dsn::replication::ballot)-1LL)
#define invalid_decree ((::dsn::replication::decree)-1LL)
#define invalid_offset (-1LL)
#define invalid_signature 0

inline bool is_primary(const partition_configuration &pc, const host_port &node)
{
    return node && pc.hp_primary == node;
}
inline bool is_primary(const partition_configuration &pc, const rpc_address &node)
{
    return node && pc.primary == node;
}
inline bool is_secondary(const partition_configuration &pc, const host_port &node)
{
    return node && utils::contains(pc.hp_secondaries, node);
}
inline bool is_secondary(const partition_configuration &pc, const rpc_address &node)
{
    return node && utils::contains(pc.secondaries, node);
}
inline bool is_member(const partition_configuration &pc, const host_port &node)
{
    return is_primary(pc, node) || is_secondary(pc, node);
}
inline bool is_member(const partition_configuration &pc, const rpc_address &node)
{
    return is_primary(pc, node) || is_secondary(pc, node);
}
inline bool is_partition_config_equal(const partition_configuration &pc1,
                                      const partition_configuration &pc2)
{
    // secondaries no need to be same order
    for (const auto &pc1_secondary : pc1.hp_secondaries) {
        if (!is_secondary(pc2, pc1_secondary)) {
            return false;
        }
    }
    // last_drops is not considered into equality check
    return pc1.ballot == pc2.ballot && pc1.pid == pc2.pid &&
           pc1.max_replica_count == pc2.max_replica_count && pc1.primary == pc2.primary &&
           pc1.hp_primary == pc2.hp_primary && pc1.secondaries.size() == pc2.secondaries.size() &&
           pc1.hp_secondaries.size() == pc2.hp_secondaries.size() &&
           pc1.last_committed_decree == pc2.last_committed_decree;
}

class replica_helper
{
public:
    static bool get_replica_config(const partition_configuration &pc,
                                   const ::dsn::host_port &node,
                                   /*out*/ replica_configuration &rc);

    // Return true if 'server_list' is a valid comma-separated list of servers, otherwise return
    // false. The result is filled into 'servers' if success.
    static bool parse_server_list(const char *server_list,
                                  /*out*/ std::vector<dsn::host_port> &servers);

    // Return true if load server list from config value located in 'section' and 'key', the value
    // should be a valid comma-separated list of servers, otherwise return false. The result is
    // filled into 'servers' if success.
    static bool load_servers_from_config(const std::string &section,
                                         const std::string &key,
                                         /*out*/ std::vector<dsn::host_port> &servers);
};
} // namespace replication
} // namespace dsn

#endif
