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

#include "dsn_message_parser.h"
#include "runtime/api_task.h"
#include "runtime/api_layer1.h"
#include "runtime/app_model.h"
#include "utils/api_utilities.h"
#include "utils/crc.h"

namespace dsn {
namespace {
unsigned int get_body_length(char *hdr) { return ((message_header *)hdr)->body_length; }

bool is_right_header(char *hdr)
{
    uint32_t *pcrc = reinterpret_cast<uint32_t *>(hdr + FIELD_OFFSET(message_header, hdr_crc32));
    uint32_t crc32 = *pcrc;
    if (crc32 == CRC_INVALID) {
        // crc is not enabled
        return true;
    }

    *pcrc = CRC_INVALID;
    bool r = (crc32 == dsn::utils::crc32_calc(hdr, sizeof(message_header), 0));
    *pcrc = crc32;
    if (dsn_unlikely(!r)) {
        LOG_ERROR("dsn message header crc check failed");
    }
    return r;
}

bool is_right_body(message_ex *msg)
{
    const auto &header = msg->header;
    if (header->body_crc32 == CRC_INVALID) {
        // crc is not enabled
        return true;
    }

    const auto &buffers = msg->buffers;
    uint32_t crc32 = 0;
    size_t len = 0;
    for (const auto &buffer : buffers) {
        const void *ptr = (const void *)buffer.data();
        size_t sz = (size_t)buffer.length();

        uint32_t lcrc = dsn::utils::crc32_calc(ptr, sz, crc32);
        crc32 = dsn::utils::crc32_concat(0, 0, crc32, len, crc32, lcrc, sz);

        len += sz;
    }

    CHECK_EQ(len, header->body_length);
    bool r = (header->body_crc32 == crc32);
    if (dsn_unlikely(!r)) {
        LOG_ERROR("dsn message body crc check failed");
    }
    return r;
}
} // anonymous namespace

void dsn_message_parser::reset() { _header_checked = false; }

message_ex *dsn_message_parser::get_message_on_receive(message_reader *reader,
                                                       /*out*/ int &read_next)
{
    read_next = 4096;

    dsn::blob &buf = reader->_buffer;
    char *buf_ptr = (char *)buf.data();
    unsigned int buf_len = reader->_buffer_occupied;
    if (buf_len < sizeof(message_header)) {
        read_next = sizeof(message_header) - buf_len;
        return nullptr;
    }

    if (!_header_checked) {
        if (dsn_unlikely(!is_right_header(buf_ptr))) {
            LOG_ERROR("dsn message header check failed");
            read_next = -1;
            return nullptr;
        }

        _header_checked = true;
    }

    unsigned int msg_sz = sizeof(message_header) + get_body_length(buf_ptr);
    if (buf_len < msg_sz) {
        read_next = msg_sz - buf_len;
        return nullptr;
    }

    dsn::blob msg_bb = buf.range(0, msg_sz);
    message_ex *msg = message_ex::create_receive_message(msg_bb);
    if (dsn_unlikely(!is_right_body(msg))) {
        message_header *header = (message_header *)buf_ptr;
        LOG_ERROR_F("dsn message body check failed, id = {:#018x}, trace_id = {:#018x},"
                    " rpc_name = {}, from_addr = {}",
                    header->id,
                    header->trace_id,
                    header->rpc_name,
                    header->from_address);
        read_next = -1;
        delete msg;
        return nullptr;
    }

    // TODO: use void consume_buffer(size_t sz) ?
    reader->_buffer = buf.range(msg_sz);
    reader->_buffer_occupied -= msg_sz;

    _header_checked = false;
    read_next = (reader->_buffer_occupied >= sizeof(message_header)
                     ? 0
                     : sizeof(message_header) - reader->_buffer_occupied);
    // TODO: move to the place where defined?
    msg->hdr_format = NET_HDR_DSN;
    return msg;
}

void dsn_message_parser::prepare_on_send(message_ex *msg)
{
    auto &header = msg->header;
    auto &buffers = msg->buffers;

#ifndef NDEBUG
    size_t len = 0;
    for (const auto &buffer : buffers) {
        len += buffer.length();
    }
    CHECK_EQ(len, header->body_length + sizeof(message_header));
#endif

    if (!task_spec::get(msg->local_rpc_code)->rpc_message_crc_required) {
        return;
    }

    // compute data crc if necessary (only once for the first time)
    if (header->body_crc32 == CRC_INVALID) {
        uint32_t crc32 = 0;
        size_t len = 0;
        for (int i = 0; i < buffers.size(); i++) {
            const void *ptr = nullptr;
            size_t sz = 0;
            if (i == 0) {
                ptr = (const void *)(buffers[i].data() + sizeof(message_header));
                sz = (size_t)buffers[i].length() - sizeof(message_header);
            } else {
                ptr = (const void *)buffers[i].data();
                sz = (size_t)buffers[i].length();
            }

            uint32_t lcrc = dsn::utils::crc32_calc(ptr, sz, crc32);
            crc32 = dsn::utils::crc32_concat(0, 0, crc32, len, crc32, lcrc, sz);

            len += sz;
        }

        CHECK_EQ(len, header->body_length);
        header->body_crc32 = crc32;
    }

    // always compute header crc
    header->hdr_crc32 = CRC_INVALID;
    header->hdr_crc32 = dsn::utils::crc32_calc(header, sizeof(message_header), 0);
}

int dsn_message_parser::get_buffers_on_send(message_ex *msg, /*out*/ send_buf *buffers)
{
    int i = 0;
    for (auto &buf : msg->buffers) {
        buffers[i].buf = (void *)buf.data();
        buffers[i].sz = buf.length();
        ++i;
    }
    return i;
}
}
