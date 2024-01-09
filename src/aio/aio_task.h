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
#include <functional>
#include <memory>
#include <vector>

#include "runtime/api_task.h"
#include "runtime/task/task.h"
#include "runtime/task/task_code.h"
#include "utils/autoref_ptr.h"
#include "utils/blob.h"

namespace dsn {
class error_code;
class service_node;

namespace utils {
class latency_tracer;
}

enum class rw_type
{
    kInvalid,
    kRead,
    kWrite
};

// TODO(yingchun): can be replaced by string_view?
typedef struct
{
    void *buffer;
    int size;
} dsn_file_buffer_t;

class disk_engine;
class disk_file;

class rw_context : public ref_counter
{
public:
    // filled by apps
    void *buffer = nullptr;
    uint64_t buffer_size = 0;
    uint64_t file_offset = 0;

    // filled by frameworks
    rw_type type = rw_type::kInvalid;
    disk_engine *engine = nullptr;
    disk_file *dfile = nullptr;
};
typedef dsn::ref_ptr<rw_context> rw_context_ptr;

class rw_task : public task
{
public:
    rw_task(task_code code, const rw_handler &cb, int hash = 0, service_node *node = nullptr);
    rw_task(task_code code, rw_handler &&cb, int hash = 0, service_node *node = nullptr);

    // tell the compiler that we want both the enqueue from base task and ours
    // to prevent the compiler complaining -Werror,-Woverloaded-virtual.
    using task::enqueue;
    void enqueue(error_code err, size_t transferred_size);

    size_t get_transferred_size() const { return _transferred_size; }

    // The ownership of `rw_context` is held by `rw_task`.
    rw_context *get_rw_context() { return _rw_ctx.get(); }

    // merge buffers in _unmerged_write_buffers to a single merged buffer.
    // and store it in _merged_write_buffer_holder.
    void collapse();

    // invoked on R/W operation completed
    virtual void exec() override
    {
        if (nullptr != _cb) {
            _cb(_error, _transferred_size);
        }
    }

    std::vector<dsn_file_buffer_t> _unmerged_write_buffers;
    blob _merged_write_buffer_holder;
    std::shared_ptr<dsn::utils::latency_tracer> _tracer;

protected:
    void clear_non_trivial_on_task_end() override { _cb = nullptr; }

private:
    rw_context_ptr _rw_ctx;
    size_t _transferred_size;
    rw_handler _cb;
};
typedef dsn::ref_ptr<rw_task> rw_task_ptr;

} // namespace dsn
