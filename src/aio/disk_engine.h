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

#include <stddef.h>
#include <stdint.h>
#include <memory>

#include "aio/aio_task.h"
#include "aio_provider.h"
#include "rocksdb/env.h"
#include "utils/singleton.h"
#include "utils/work_queue.h"

namespace dsn {
class error_code;

class disk_write_queue : public work_queue<rw_task>
{
public:
    disk_write_queue() : work_queue(2)
    {
        _max_batch_bytes = 1024 * 1024; // 1 MB
    }

private:
    virtual rw_task *unlink_next_workload(void *plength) override;

private:
    uint32_t _max_batch_bytes;
};

class disk_file
{
public:
    explicit disk_file(std::unique_ptr<rocksdb::RandomAccessFile> rf);
    explicit disk_file(std::unique_ptr<rocksdb::RandomRWFile> wf);
    rw_task *read(rw_task *tsk);
    rw_task *write(rw_task *tsk, void *ctx);

    rw_task *on_read_completed(rw_task *wk, error_code err, size_t size);
    rw_task *on_write_completed(rw_task *wk, void *ctx, error_code err, size_t size);

    rocksdb::RandomAccessFile *rfile() const { return _read_file.get(); }
    rocksdb::RandomRWFile *wfile() const { return _write_file.get(); }

private:
    // TODO(yingchun): unify to use a single RandomRWFile member variable.
    std::unique_ptr<rocksdb::RandomAccessFile> _read_file;
    std::unique_ptr<rocksdb::RandomRWFile> _write_file;
    disk_write_queue _write_queue;
    work_queue<rw_task> _read_queue;
};

class disk_engine : public utils::singleton<disk_engine>
{
public:
    void write(rw_task *rw_tsk);
    static rw_provider &provider() { return *instance()._provider.get(); }

private:
    // the object of disk_engine must be created by `singleton::instance`
    disk_engine();
    ~disk_engine() = default;

    void process_write(rw_task *rw_tsk, uint64_t sz);
    void complete_io(rw_task *rw_tsk, error_code err, uint64_t bytes);

    std::unique_ptr<rw_provider> _provider;

    friend class rw_provider;
    friend class batch_write_io_task;
    friend class utils::singleton<disk_engine>;
};

} // namespace dsn
