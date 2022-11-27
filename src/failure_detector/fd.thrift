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

include "../../idl/dsn.thrift"

namespace cpp dsn.fd

struct beacon_msg
{
    1: i64 time;
    2: dsn.rpc_address from_addr;
    3: dsn.rpc_address to_addr;
    4: optional i64 start_time;
    5: dsn.host_port from_host_port;
    6: dsn.host_port to_host_port;
}

struct beacon_ack
{
    1: i64 time;
    2: dsn.rpc_address this_node;
    3: dsn.rpc_address primary_node;
    4: bool is_master;
    5: bool allowed;
    6: dsn.host_port this_node_host_port;
    7: dsn.host_port primary_node_host_port;
}

struct config_master_message
{
    1: dsn.rpc_address master;
    2: bool is_register;
    3: dsn.host_port host_port_master;
}
