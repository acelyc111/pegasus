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

#include "disk_engine.h"

#include <list>
// IWYU pragma: no_include <string>
#include <utility>
#include <vector>

#include "aio/aio_provider.h"
#include "aio/aio_task.h"
#include "native_linux_aio_provider.h"
#include "runtime/task/task.h"
#include "runtime/task/task_code.h"
#include "runtime/task/task_spec.h"
#include "runtime/tool_api.h"
#include "utils/error_code.h"
#include "utils/factory_store.h"
#include "utils/fmt_logging.h"
#include "utils/join_point.h"
#include "utils/link.h"
#include "utils/threadpool_code.h"

namespace dsn {
DEFINE_TASK_CODE_AIO(LPC_AIO_BATCH_WRITE, TASK_PRIORITY_COMMON, THREAD_POOL_DEFAULT)

const char *native_rw_provider = "dsn::tools::native_aio_provider";
DSN_REGISTER_COMPONENT_PROVIDER(native_linux_rw_provider, native_rw_provider);

struct disk_engine_initializer
{
    disk_engine_initializer() { disk_engine::instance(); }
};

// make disk_engine destructed after service_engine, which is inited in dsn_global_init,
// because service_engine relies on the former to close files.
static disk_engine_initializer disk_engine_init;

//----------------- disk_file ------------------------
rw_task *disk_write_queue::unlink_next_workload(void *plength)
{
    uint64_t next_offset = 0;
    uint64_t &sz = *(uint64_t *)plength;
    sz = 0;

    rw_task *first = _hdr._first, *current = first, *last = first;
    while (nullptr != current) {
        auto io = current->get_rw_context();
        if (sz == 0) {
            sz = io->buffer_size;
            next_offset = io->file_offset + sz;
        } else {
            // batch condition
            if (next_offset == io->file_offset && sz + io->buffer_size <= _max_batch_bytes) {
                sz += io->buffer_size;
                next_offset += io->buffer_size;
            }

            // no batch is possible
            else {
                break;
            }
        }

        // continue next
        last = current;
        current = (rw_task *)current->next;
    }

    // unlink [first, last] -> current
    if (last) {
        _hdr._first = current;
        if (last == _hdr._last)
            _hdr._last = nullptr;
        last->next = nullptr;
    }

    return first;
}

disk_file::disk_file(std::unique_ptr<rocksdb::RandomAccessFile> rf) : _read_file(std::move(rf)) {}
disk_file::disk_file(std::unique_ptr<rocksdb::RandomRWFile> wf) : _write_file(std::move(wf)) {}

rw_task *disk_file::read(rw_task *tsk)
{
    CHECK(_read_file, "");
    tsk->add_ref(); // release on completion, see `on_read_completed`.
    return _read_queue.add_work(tsk, nullptr);
}

rw_task *disk_file::write(rw_task *tsk, void *ctx)
{
    CHECK(_write_file, "");
    tsk->add_ref(); // release on completion
    return _write_queue.add_work(tsk, ctx);
}

rw_task *disk_file::on_read_completed(rw_task *wk, error_code err, size_t size)
{
    CHECK(_read_file, "");
    CHECK(wk->next == nullptr, "");
    auto ret = _read_queue.on_work_completed(wk, nullptr);
    wk->enqueue(err, size);
    wk->release_ref(); // added in above read

    return ret;
}

rw_task *disk_file::on_write_completed(rw_task *wk, void *ctx, error_code err, size_t size)
{
    CHECK(_write_file, "");
    auto ret = _write_queue.on_work_completed(wk, ctx);

    while (wk) {
        rw_task *next = (rw_task *)wk->next;
        wk->next = nullptr;

        if (err == ERR_OK) {
            size_t this_size = (size_t)wk->get_rw_context()->buffer_size;
            CHECK_GE(size, this_size);
            wk->enqueue(err, this_size);
            size -= this_size;
        } else {
            wk->enqueue(err, size);
        }

        wk->release_ref(); // added in above write

        wk = next;
    }

    if (err == ERR_OK) {
        CHECK_EQ_MSG(size, 0, "written buffer size does not equal to input buffer's size");
    }

    return ret;
}

//----------------- disk_engine ------------------------
disk_engine::disk_engine()
{
    rw_provider *provider = utils::factory_store<rw_provider>::create(
        native_rw_provider, dsn::PROVIDER_TYPE_MAIN, this);
    _provider.reset(provider);
}

class batch_write_io_task : public rw_task
{
public:
    explicit batch_write_io_task(rw_task *tasks)
        : rw_task(LPC_AIO_BATCH_WRITE, nullptr), _tasks(tasks)
    {
    }

    virtual void exec() override
    {
        auto dfile = _tasks->get_rw_context()->dfile;
        uint64_t sz;
        auto wk = dfile->on_write_completed(_tasks, (void *)&sz, error(), get_transferred_size());
        if (wk) {
            wk->get_rw_context()->engine->process_write(wk, sz);
        }
    }

public:
    rw_task *_tasks;
};

void disk_engine::write(rw_task *rw_tsk)
{
    if (!rw_tsk->spec().on_aio_call.execute(task::get_current_task(), rw_tsk, true)) {
        rw_tsk->enqueue(ERR_FILE_OPERATION_FAILED, 0);
        return;
    }

    auto dio = rw_tsk->get_rw_context();
    dio->engine = this;

    uint64_t sz;
    auto wk = dio->dfile->write(rw_tsk, &sz);
    if (wk) {
        process_write(wk, sz);
    }
}

void disk_engine::process_write(rw_task *rw_tsk, uint64_t sz)
{
    rw_context *dio = rw_tsk->get_rw_context();

    // no batching
    if (dio->buffer_size == sz) {
        rw_tsk->collapse();
        _provider->submit_rw_task(rw_tsk);
    }

    // batching
    else {
        // setup io task
        auto new_task = new batch_write_io_task(rw_tsk);
        auto new_dio = new_task->get_rw_context();
        new_dio->buffer_size = sz;
        new_dio->file_offset = dio->file_offset;
        new_dio->dfile = dio->dfile;
        new_dio->engine = dio->engine;
        new_dio->type = rw_type::kWrite;

        auto cur_task = rw_tsk;
        do {
            auto cur_dio = cur_task->get_rw_context();
            if (cur_dio->buffer) {
                dsn_file_buffer_t buf;
                buf.buffer = cur_dio->buffer;
                buf.size = cur_dio->buffer_size;
                new_task->_unmerged_write_buffers.push_back(std::move(buf));
            } else {
                new_task->_unmerged_write_buffers.insert(new_task->_unmerged_write_buffers.end(),
                                                         cur_task->_unmerged_write_buffers.begin(),
                                                         cur_task->_unmerged_write_buffers.end());
            }
            cur_task = (rw_task *)cur_task->next;
        } while (cur_task);

        new_task->add_ref(); // released in complete_io
        process_write(new_task, sz);
    }
}

void disk_engine::complete_io(rw_task *rw_tsk, error_code err, uint64_t bytes)
{
    if (err != ERR_OK) {
        LOG_DEBUG("disk operation failure with code {}, err = {}, rw_task_id = {:#018x}",
                  rw_tsk->spec().name,
                  err,
                  rw_tsk->id());
    }

    // batching
    if (rw_tsk->code() == LPC_AIO_BATCH_WRITE) {
        rw_tsk->enqueue(err, (size_t)bytes);
        rw_tsk->release_ref(); // added in process_write
    }

    // no batching
    else {
        auto dfile = rw_tsk->get_rw_context()->dfile;
        if (rw_tsk->get_rw_context()->type == rw_type::kRead) {
            auto wk = dfile->on_read_completed(rw_tsk, err, (size_t)bytes);
            if (wk) {
                _provider->submit_rw_task(wk);
            }
        }

        // write
        else {
            uint64_t sz;
            auto wk = dfile->on_write_completed(rw_tsk, (void *)&sz, err, (size_t)bytes);
            if (wk) {
                process_write(wk, sz);
            }
        }
    }
}
} // namespace dsn
