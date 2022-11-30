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
#include "runtime/rpc/rpc_host_port.h"
#include "utils/rand.h"

namespace dsn {
namespace dist {

slave_failure_detector_with_multimaster::slave_failure_detector_with_multimaster(
    const std::shared_ptr<dns_resolver> &resolver,
    const host_port_group &meta_servers,
    std::function<void()> &&master_disconnected_callback,
    std::function<void()> &&master_connected_callback)
    : dsn::fd::failure_detector(resolver), _meta_servers(meta_servers)
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
    LOG_INFO_F("end ping result, error[{}], time[{}], ack.this_node[{}({})], "
               "ack.primary_node[{}({})], __isset: {}:{}, ack.is_master[{}], ack.allowed[{}]",
               err,
               ack.time,
               ack.host_port_this_node,
               ack.this_node,
               ack.host_port_primary_node,
               ack.primary_node,
               ack.__isset.host_port_this_node,
               ack.__isset.this_node,
               ack.is_master ? "true" : "false",
               ack.allowed ? "true" : "false");

    zauto_lock l(failure_detector::_lock);
    if (!failure_detector::end_ping_internal(err, ack)) {
        return;
    }

    host_port this_node;
    if (ack.__isset.host_port_this_node) {
        this_node = ack.host_port_this_node;
    } else {
        CHECK(ack.__isset.this_node, "");
        this_node = host_port(ack.this_node);
    }
    CHECK_EQ(this_node, _meta_servers.leader());

    // TODO(yingchun): refactor the if-else statements
    // Try to send beacon to the next master when current master return error.
    if (ERR_OK != err) {
        host_port next = _meta_servers.next(this_node);
        if (next != this_node) {
            _meta_servers.set_leader(next);
            // Do not start next send_beacon() immediately to avoid send rpc too frequently
            switch_master(this_node, next, 1000);
        }
        return;
    }

    // Do nothing if the current master not changed
    if (ack.is_master) {
        return;
    }

    host_port primary_node;
    if (ack.__isset.host_port_primary_node) {
        primary_node = ack.host_port_primary_node;
    } else {
        CHECK(ack.__isset.primary_node, "");
        primary_node = host_port(ack.primary_node);
    }

    // Try to send beacon to the next master when master changed and the hint master is invalid
    if (!primary_node.initialized()) {
        host_port next = _meta_servers.next(this_node);
        if (next != this_node) {
            _meta_servers.set_leader(next);
            // do not start next send_beacon() immediately to avoid send rpc too frequently
            switch_master(this_node, next, 1000);
        }
        return;
    }

    // Try to send beacon to the hint master when master changed and give a valid hint master
    _meta_servers.set_leader(primary_node);
    // start next send_beacon() immediately because the leader is possibly right.
    switch_master(this_node, primary_node, 0);
}

// client side
void slave_failure_detector_with_multimaster::on_master_disconnected(
    const std::vector<host_port> &nodes)
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

void slave_failure_detector_with_multimaster::on_master_connected(const host_port &node)
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
