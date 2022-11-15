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

#pragma once

#include "runtime/tool_api.h"
#include "nfs/nfs_node.h"

namespace dsn {
namespace service {

class nfs_service_impl;
class nfs_client_impl;

class nfs_node_simple : public nfs_node
{
public:
    nfs_node_simple(const std::shared_ptr<dns_resolver> &resolver);

    virtual ~nfs_node_simple();

    void call(std::shared_ptr<remote_copy_request> rci, aio_task *callback) override;

    error_code start() override;

    error_code stop() override;

private:
    std::unique_ptr<nfs_service_impl> _server;
    std::unique_ptr<nfs_client_impl> _client;
    std::shared_ptr<dns_resolver> _dns_resolver;
};
} // namespace service
} // namespace dsn
