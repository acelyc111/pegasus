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

#include <cinttypes>

#include "failure_detector/failure_detector_multimaster.h"
#include "runtime/rpc/group_address.h"
#include "runtime/rpc/rpc_address.h"
#include "utils/rand.h"

namespace dsn {
namespace dist {

slave_failure_detector_with_multimaster::slave_failure_detector_with_multimaster(
    const ::dsn::host_port_group &meta_servers,
    std::function<void()> &&master_disconnected_callback,
    std::function<void()> &&master_connected_callback)
    : _meta_servers(meta_servers)
{
    _meta_servers.set_rand_leader();
    _master_disconnected_callback = std::move(master_disconnected_callback);
    _master_connected_callback = std::move(master_connected_callback);
}

void slave_failure_detector_with_multimaster::set_leader_for_test(const host_port &hp)
{
    _meta_servers.set_leader(hp);
}

void slave_failure_detector_with_multimaster::end_ping(::dsn::error_code err,
                                                       const fd::beacon_ack &ack,
                                                       void *)
{
    // TODO(yingchun): 1. this_node_host_port and primary_node_host_port not print. 2. add
    // to_string() for beacon_ack.
    LOG_INFO_F("end ping result, error[{}], time[{}], ack.this_node[{}], ack.primary_node[{}], "
               "ack.is_master[{}], ack.allowed[{}]",
               err,
               ack.time,
               ack.this_node,
               ack.primary_node,
               ack.is_master ? "true" : "false",
               ack.allowed ? "true" : "false");

    zauto_lock l(failure_detector::_lock);
    if (!failure_detector::end_ping_internal(err, ack)) {
        return;
    }

    // TODO(yingchun): ip -> host:port
    const host_port this_node_hp; // from ack.this_node
    CHECK_EQ(this_node_hp, _meta_servers.leader());

    // TODO(yingchun): refactor the if-else statements
    // Try to send beacon to the next master when current master return error.
    if (ERR_OK != err) {
        host_port next = _meta_servers.next(this_node_hp);
        if (next != this_node_hp) {
            _meta_servers.set_leader(next);
            // Do not start next send_beacon() immediately to avoid send rpc too frequently
            switch_master(this_node_hp, next, 1000);
        }
        return;
    }

    // Do nothing if the current master not changed
    if (ack.is_master) {
        return;
    }

    // TODO(yingchun): ip -> host:port
    const host_port primary_node; // from ack.primary_node
    // Try to send beacon to the next master when master changed and the hint master is invalid
    if (!primary_node.initialized()) {
        host_port next = _meta_servers.next(this_node_hp);
        if (next != this_node_hp) {
            _meta_servers.set_leader(next);
            // do not start next send_beacon() immediately to avoid send rpc too frequently
            switch_master(this_node_hp, next, 1000);
        }
        return;
    }

    // Try to send beacon to the hint master when master changed and give a valid hint master
    _meta_servers.set_leader(primary_node);
    // start next send_beacon() immediately because the leader is possibly right.
    switch_master(this_node_hp, primary_node, 0);
}

// client side
void slave_failure_detector_with_multimaster::on_master_disconnected(
    const std::vector<::dsn::host_port> &nodes)
{
    bool primary_disconnected = false;
    host_port leader = _meta_servers.leader();
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        if (leader == *it) {
            primary_disconnected = true;
            break;
        }
    }

    if (primary_disconnected) {
        _master_disconnected_callback();
    }
}

void slave_failure_detector_with_multimaster::on_master_connected(const ::dsn::host_port &node)
{
    /*
    * well, this is called in on_ping_internal, which is called by rep::end_ping.
    * So this function is called in the lock context of fd::_lock
    */
    if (_meta_servers.leader() == node) {
        _master_connected_callback();
    }
}
}
} // end namespace
