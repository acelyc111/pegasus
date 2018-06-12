// Copyright (c) 2017, Xiaomi, Inc.  All rights reserved.
// This source code is licensed under the Apache License Version 2.0, which
// can be found in the LICENSE file in the root directory of this source tree.

#include <boost/lexical_cast.hpp>
#include <rrdb/rrdb.client.h>
#include <pegasus/error.h>
#include <rocksdb/status.h>
#include <pegasus_key_schema.h>
#include <pegasus_utils.h>
#include <dsn/utility/string_conv.h>
#include <dsn/dist/fmt_logging.h>
#include "redis_parser.h"

#define CR '\015'
#define LF '\012'

namespace pegasus {
namespace proxy {

std::unordered_map<std::string, redis_parser::redis_call_handler> redis_parser::s_dispatcher = {
    {"SET", redis_parser::g_set},
    {"SET_GEO", redis_parser::g_set_geo}, // TODO set_geo ttl
    {"GET", redis_parser::g_get},
    {"DEL", redis_parser::g_del},
    {"SETEX", redis_parser::g_setex},
    {"TTL", redis_parser::g_ttl},
    {"PTTL", redis_parser::g_ttl},
    {"GEORADIUS", redis_parser::g_geo_radius},
};

redis_parser::redis_call_handler redis_parser::get_handler(const char *command, unsigned int length)
{
    std::string key(command, length);
    std::transform(key.begin(), key.end(), key.begin(), toupper);
    auto iter = s_dispatcher.find(key);
    if (iter == s_dispatcher.end())
        return redis_parser::g_default_handler;
    return iter->second;
}

redis_parser::redis_parser(proxy_stub *op, ::dsn::rpc_address remote)
    : proxy_session(op, remote),
      next_seqid(0),
      current_msg(new message_entry()),
      status(start_array),
      current_size(),
      total_length(0),
      current_buffer(nullptr),
      current_buffer_length(0),
      current_cursor(0)
{
    ::dsn::apps::rrdb_client *r;
    if (op) {
        r = new ::dsn::apps::rrdb_client(op->get_service_uri());
        // value format:
        // "00:00:00:00:01:5e|2018-04-26|2018-04-28|ezp8xchrr|-0.356396|39.469644|24.043028|4.15921|0|-1"
        auto extractor = [](const std::string &value, S2LatLng &latlng) {
            std::vector<std::string> data;
            dsn::utils::split_args(value.c_str(), data, '|');
            if (data.size() <= 6) {
                return pegasus::PERR_INVALID_VALUE;
            }

            std::string lat = data[5];
            std::string lng = data[4];
            latlng =
                S2LatLng::FromDegrees(strtod(lat.c_str(), nullptr), strtod(lng.c_str(), nullptr));

            return pegasus::PERR_OK;
        };
        _geo_client = std::make_unique<geo::geo_client>("config.ini",
                                                        op->get_cluster(),
                                                        op->get_app(),
                                                        op->get_geo_app(),
                                                        extractor); // TODO config.ini
    } else {
        r = new ::dsn::apps::rrdb_client();
    }
    client.reset(r);
}

redis_parser::~redis_parser() { dinfo("redis parser destroyed"); }

void redis_parser::prepare_current_buffer()
{
    void *msg_buffer;
    if (current_buffer == nullptr) {
        dsn_message_t first_msg = recv_buffers.front();
        dassert(dsn_msg_read_next(first_msg, &msg_buffer, &current_buffer_length),
                "read dsn_message_t failed, msg from_address = %s, to_address = %s, rpc_name = %s",
                ((::dsn::message_ex *)first_msg)->header->from_address.to_string(),
                ((::dsn::message_ex *)first_msg)->to_address.to_string(),
                ((::dsn::message_ex *)first_msg)->header->rpc_name);
        current_buffer = reinterpret_cast<char *>(msg_buffer);
        current_cursor = 0;
    } else if (current_cursor >= current_buffer_length) {
        dsn_message_t first_msg = recv_buffers.front();
        dsn_msg_read_commit(first_msg, current_buffer_length);
        if (dsn_msg_read_next(first_msg, &msg_buffer, &current_buffer_length)) {
            current_cursor = 0;
            current_buffer = reinterpret_cast<char *>(msg_buffer);
            return;
        } else {
            // we have consume this message all over
            dsn_msg_release_ref(first_msg); // added when messaged is received in proxy_stub
            recv_buffers.pop();
            current_buffer = nullptr;
            prepare_current_buffer();
        }
    } else
        return;
}

void redis_parser::reset()
{
    // clear the response pipeline
    {
        ::dsn::service::zauto_lock l(_rlock);
        while (!pending_response.empty()) {
            message_entry *entry = pending_response.front().get();
            if (entry->response)
                dsn_msg_release_ref(entry->response);

            pending_response.pop_front();
        }
    }
    next_seqid = 0;

    // clear the parser status
    current_msg->request.length = 0;
    current_msg->request.buffers.clear();
    status = start_array;
    current_size.clear();

    // clear the data stream
    total_length = 0;
    if (current_buffer) {
        dsn_msg_read_commit(recv_buffers.front(), current_buffer_length);
    }
    current_buffer = nullptr;
    current_buffer_length = 0;
    current_cursor = 0;
    while (!recv_buffers.empty()) {
        dsn_msg_release_ref(recv_buffers.front());
        recv_buffers.pop();
    }
}

char redis_parser::peek()
{
    prepare_current_buffer();
    return current_buffer[current_cursor];
}

void redis_parser::eat(char c)
{
    prepare_current_buffer();
    if (current_buffer[current_cursor] == c) {
        ++current_cursor;
        --total_length;
    } else {
        derror("expect token: %c, got %c", c, current_buffer[current_cursor]);
        throw std::invalid_argument("");
    }
}

void redis_parser::eat_all(char *dest, size_t length)
{
    total_length -= length;
    while (length > 0) {
        prepare_current_buffer();

        size_t eat_size = current_buffer_length - current_cursor;
        if (eat_size > length)
            eat_size = length;
        memcpy(dest, current_buffer + current_cursor, eat_size);
        dest += eat_size;
        current_cursor += eat_size;
        length -= eat_size;
    }
}

void redis_parser::end_array_size()
{
    redis_request &current_array = current_msg->request;
    try {
        current_array.length = boost::lexical_cast<int>(current_size);
        current_size.clear();
        if (current_array.length <= 0) {
            derror("array size should be positive in redis request, but %d", current_array.length);
            throw std::invalid_argument("");
        }
        current_array.buffers.reserve(current_array.length);
        status = start_bulk_string;
    } catch (const boost::bad_lexical_cast &c) {
        derror("invalid size string \"%s\"", current_size.c_str());
        throw std::invalid_argument("");
    }
}

void redis_parser::append_current_bulk_string()
{
    redis_request &current_array = current_msg->request;
    current_array.buffers.push_back(current_str);
    if (current_array.buffers.size() == current_array.length) {
        // we get a full request command
        handle_command(std::move(current_msg));
        current_msg.reset(new message_entry());
        status = start_array;
    } else {
        status = start_bulk_string;
    }
}

void redis_parser::end_bulk_string_size()
{
    try {
        current_str.length = boost::lexical_cast<int>(current_size);
        current_str.data.assign(nullptr, 0, 0);
        current_size.clear();
        if (-1 == current_str.length) {
            append_current_bulk_string();
        } else if (current_str.length >= 0) {
            status = start_bulk_string_data;
        } else {
            derror("invalid bulk string length: %d", current_str.length);
            throw std::invalid_argument("");
        }
    } catch (const boost::bad_lexical_cast &c) {
        derror("invalid size string \"%s\"", current_size.c_str());
        throw std::invalid_argument("");
    }
}

void redis_parser::append_message(dsn_message_t msg)
{
    recv_buffers.push(msg);
    total_length += dsn_msg_body_size(msg);
    dinfo("recv message, currently total length is %d", total_length);
}

// refererence: http://redis.io/topics/protocol
void redis_parser::parse_stream()
{
    char t;
    while (total_length > 0) {
        switch (status) {
        case start_array:
            eat('*');
            status = in_array_size;
            break;
        case in_array_size:
        case in_bulk_string_size:
            t = peek();
            if (t == CR) {
                if (total_length > 1) {
                    eat(CR);
                    eat(LF);
                    if (in_array_size == status)
                        end_array_size();
                    else
                        end_bulk_string_size();
                } else
                    return;
            } else {
                current_size.push_back(t);
                eat(t);
            }
            break;
        case start_bulk_string:
            eat('$');
            status = in_bulk_string_size;
            break;
        case start_bulk_string_data:
            // string content + CR + LF
            if (total_length >= current_str.length + 2) {
                if (current_str.length > 0) {
                    char *ptr = reinterpret_cast<char *>(dsn::tls_trans_malloc(current_str.length));
                    std::shared_ptr<char> str_data(ptr,
                                                   [](char *ptr) { dsn::tls_trans_free(ptr); });
                    eat_all(str_data.get(), current_str.length);
                    current_str.data.assign(std::move(str_data), 0, current_str.length);
                }
                eat(CR);
                eat(LF);
                append_current_bulk_string();
            } else
                return;
            break;
        default:
            break;
        }
    }
}

bool redis_parser::parse(dsn_message_t msg)
{
    append_message(msg);
    try {
        parse_stream();
        return true;
    } catch (...) {
        reset();
        return false;
    }
}

void redis_parser::on_remove_session(std::shared_ptr<proxy_session> _this)
{
    reset();
    status = removed;
}

void redis_parser::reply_all_ready()
{
    _rlock.lock();
    while (!pending_response.empty()) {
        message_entry *entry = pending_response.front().get();
        if (!entry->response) {
            _rlock.unlock();
            return;
        }
        std::unique_ptr<message_entry> e = std::move(pending_response.front());
        pending_response.pop_front();
        _rlock.unlock();
        dsn_rpc_reply(e->response, ::dsn::ERR_OK);
        // added when message is created
        dsn_msg_release_ref(e->response);
        _rlock.lock();
    }
    _rlock.unlock();
}

void redis_parser::default_handler(redis_parser::message_entry &entry)
{
    ::dsn::blob &cmd = entry.request.buffers[0].data;
    redis_simple_string result;
    result.is_error = true;
    result.message = "ERR unknown command '" + std::string(cmd.data(), cmd.length()) + "'";
    reply_message(entry, result);
}

void redis_parser::set(redis_parser::message_entry &entry)
{
    redis_request &request = entry.request;
    if (request.buffers.size() < 3) {
        redis_simple_string result;
        result.is_error = true;
        result.message = "ERR wrong number of arguments for 'set' command";
        reply_message(entry, result);
    } else {
        // with a reference to prevent the object from being destoryed
        std::shared_ptr<proxy_session> ref_this = shared_from_this();
        auto on_set_reply =
            [ref_this, this, &entry](::dsn::error_code ec, dsn_message_t, dsn_message_t response) {
                if (status == removed)
                    return;

                if (::dsn::ERR_OK != ec) {
                    redis_simple_string result;
                    result.is_error = true;
                    result.message = std::string("ERR ") + ec.to_string();
                    reply_message(entry, result);
                } else {
                    ::dsn::apps::update_response rrdb_response;
                    ::dsn::unmarshall(response, rrdb_response);
                    if (rrdb_response.error != 0) {
                        redis_simple_string result;
                        result.is_error = true;
                        result.message = "ERR internal error " +
                                         boost::lexical_cast<std::string>(rrdb_response.error);
                        reply_message(entry, result);
                    } else {
                        redis_simple_string result;
                        result.is_error = false;
                        result.message = "OK";
                        reply_message(entry, result);
                    }
                }
            };

        ::dsn::apps::update_request req;
        ::dsn::blob null_blob;
        pegasus_generate_key(req.key, request.buffers[1].data, null_blob);
        req.value = request.buffers[2].data;
        req.expire_ts_seconds = 0;
        auto partition_hash = pegasus_key_hash(req.key);
        // TODO: set the timeout
        client->put(req,
                    on_set_reply,
                    std::chrono::milliseconds(2000),
                    0,
                    partition_hash,
                    proxy_session::hash());
    }
}

void redis_parser::set_geo(message_entry &entry)
{
    redis_request &redis_request = entry.request;
    if (redis_request.buffers.size() < 3) {
        redis_simple_string result;
        result.is_error = true;
        result.message = "ERR wrong number of arguments for 'set_geo' command";
        reply_message(entry, result);
    } else {
        // with a reference to prevent the object from being destoryed
        std::shared_ptr<proxy_session> ref_this = shared_from_this();

        // TODO: set the timeout
        _geo_client->async_set(
            redis_request.buffers[1].data.to_string(),
            std::string(),
            redis_request.buffers[2].data.to_string(),
            [ref_this, this, &entry](int ec, pegasus_client::internal_info &&info) {
                if (status == removed) {
                    return;
                }

                if (PERR_OK != ec) {
                    redis_simple_string result;
                    result.is_error = true;
                    result.message = std::string("ERR ") + _geo_client->get_error_string(ec);
                    reply_message(entry, result);
                } else {
                    redis_simple_string result;
                    result.is_error = false;
                    result.message = "OK";
                    reply_message(entry, result);
                }
            },
            2000,
            0);
    }
}

void redis_parser::setex(message_entry &entry)
{
    redis_request &redis_req = entry.request;
    // setex key ttl_SECONDS value
    if (redis_req.buffers.size() != 4) {
        redis_simple_string result;
        result.is_error = true;
        result.message = "ERR wrong number of arguments for 'setex' command";
        reply_message(entry, result);
    } else {
        redis_simple_string result;
        ::dsn::blob &ttl_blob = redis_req.buffers[2].data;
        int ttl_seconds;
        if (!pegasus::utils::buf2int(ttl_blob.data(), ttl_blob.length(), ttl_seconds)) {
            result.is_error = true;
            result.message = "ERR value is not an integer or out of range";
            reply_message(entry, result);
            return;
        }
        if (ttl_seconds <= 0) {
            result.is_error = true;
            result.message = "ERR invalid expire time in setex";
            reply_message(entry, result);
            return;
        }

        std::shared_ptr<proxy_session> ref_this = shared_from_this();
        auto on_setex_reply = [ref_this, this, &entry](
            ::dsn::error_code ec, dsn_message_t, dsn_message_t response) {
            if (status == removed)
                return;

            redis_simple_string result;
            if (::dsn::ERR_OK != ec) {
                result.is_error = true;
                result.message = std::string("ERR ") + ec.to_string();
                reply_message(entry, result);
                return;
            }

            ::dsn::apps::update_response rrdb_response;
            ::dsn::unmarshall(response, rrdb_response);
            if (rrdb_response.error != 0) {
                result.is_error = true;
                result.message =
                    "ERR internal error " + boost::lexical_cast<std::string>(rrdb_response.error);
                reply_message(entry, result);
                return;
            }

            result.is_error = false;
            result.message = "OK";
            reply_message(entry, result);
        };

        ::dsn::apps::update_request req;
        ::dsn::blob null_blob;

        pegasus_generate_key(req.key, redis_req.buffers[1].data, null_blob);
        req.value = redis_req.buffers[3].data;
        req.expire_ts_seconds = pegasus::utils::epoch_now() + ttl_seconds;

        auto partition_hash = pegasus_key_hash(req.key);

        // TODO: set the timeout
        client->put(req,
                    on_setex_reply,
                    std::chrono::milliseconds(2000),
                    0,
                    partition_hash,
                    proxy_session::hash());
    }
}

void redis_parser::get(message_entry &entry)
{
    redis_request &redis_req = entry.request;
    if (redis_req.buffers.size() != 2) {
        redis_simple_string result;
        result.is_error = true;
        result.message = "ERR wrong number of arguments for 'get' command";
        reply_message(entry, result);
    } else {
        std::shared_ptr<proxy_session> ref_this = shared_from_this();
        auto on_get_reply =
            [ref_this, this, &entry](::dsn::error_code ec, dsn_message_t, dsn_message_t response) {
                if (removed == status)
                    return;

                if (::dsn::ERR_OK != ec) {
                    redis_simple_string result;
                    result.is_error = true;
                    result.message = std::string("ERR ") + ec.to_string();
                    reply_message(entry, result);
                } else {
                    ::dsn::apps::read_response rrdb_response;
                    ::dsn::unmarshall(response, rrdb_response);
                    if (rrdb_response.error != 0) {
                        if (rrdb_response.error == rocksdb::Status::kNotFound) {
                            redis_bulk_string result;
                            result.length = -1;
                            reply_message(entry, result);
                        } else {
                            redis_simple_string result;
                            result.is_error = true;
                            result.message = "ERR internal error " +
                                             boost::lexical_cast<std::string>(rrdb_response.error);
                            reply_message(entry, result);
                        }
                    } else {
                        redis_bulk_string result(rrdb_response.value);
                        reply_message(entry, result);
                    }
                }
            };
        ::dsn::blob req;
        ::dsn::blob null_blob;
        pegasus_generate_key(req, redis_req.buffers[1].data, null_blob);
        auto partition_hash = pegasus_key_hash(req);
        // TODO: set the timeout
        client->get(req,
                    on_get_reply,
                    std::chrono::milliseconds(2000),
                    0,
                    partition_hash,
                    proxy_session::hash());
    }
}

void redis_parser::del(message_entry &entry)
{
    redis_request &redis_req = entry.request;
    if (redis_req.buffers.size() != 2) {
        redis_simple_string result;
        result.is_error = true;
        result.message = "ERR wrong number of arguments for 'del' command";
        reply_message(entry, result);
    } else {
        std::shared_ptr<proxy_session> ref_this = shared_from_this();
        auto on_del_reply =
            [ref_this, this, &entry](::dsn::error_code ec, dsn_message_t, dsn_message_t response) {
                if (removed == status)
                    return;

                if (::dsn::ERR_OK != ec) {
                    redis_simple_string result;
                    result.is_error = true;
                    result.message = std::string("ERR ") + ec.to_string();
                    reply_message(entry, result);
                } else {
                    ::dsn::apps::read_response rrdb_response;
                    ::dsn::unmarshall(response, rrdb_response);
                    if (rrdb_response.error != 0) {
                        redis_simple_string result;
                        result.is_error = true;
                        result.message = "ERR internal error " +
                                         boost::lexical_cast<std::string>(rrdb_response.error);
                        reply_message(entry, result);
                    } else {
                        redis_integer result;
                        result.value = 1;
                        reply_message(entry, result);
                    }
                }
            };
        ::dsn::blob req;
        ::dsn::blob null_blob;
        pegasus_generate_key(req, redis_req.buffers[1].data, null_blob);
        auto partition_hash = pegasus_key_hash(req);
        // TODO: set the timeout
        client->remove(req,
                       on_del_reply,
                       std::chrono::milliseconds(2000),
                       0,
                       partition_hash,
                       proxy_session::hash());
    }
}

// process 'ttl' and 'pttl'
void redis_parser::ttl(message_entry &entry)
{
    redis_request &redis_req = entry.request;
    bool is_ttl = (toupper(redis_req.buffers[0].data.data()[0]) == 'T');
    if (redis_req.buffers.size() != 2) {
        redis_simple_string result;
        result.is_error = true;
        if (is_ttl)
            result.message = "ERR wrong number of arguments for 'ttl' command";
        else
            result.message = "ERR wrong number of arguments for 'pttl' command";
        reply_message(entry, result);
    } else {
        std::shared_ptr<proxy_session> ref_this = shared_from_this();
        auto on_ttl_reply = [ref_this, this, &entry, is_ttl](
            ::dsn::error_code ec, dsn_message_t, dsn_message_t response) {
            if (removed == status)
                return;

            if (::dsn::ERR_OK != ec) {
                redis_simple_string result;
                result.is_error = true;
                result.message = std::string("ERR ") + ec.to_string();
                reply_message(entry, result);
            } else {
                ::dsn::apps::ttl_response rrdb_response;
                ::dsn::unmarshall(response, rrdb_response);
                if (rrdb_response.error != 0) {
                    if (rrdb_response.error == rocksdb::Status::kNotFound) {
                        redis_integer result;
                        result.value = -2;
                        reply_message(entry, result);
                    } else {
                        redis_simple_string result;
                        result.is_error = true;
                        result.message = "ERR internal error " +
                                         boost::lexical_cast<std::string>(rrdb_response.error);
                        reply_message(entry, result);
                    }
                } else {
                    redis_integer result;
                    if (is_ttl)
                        result.value = rrdb_response.ttl_seconds;
                    else // pttl
                        result.value = rrdb_response.ttl_seconds * 1000;
                    reply_message(entry, result);
                }
            }
        };
        ::dsn::blob req;
        ::dsn::blob null_blob;
        pegasus_generate_key(req, redis_req.buffers[1].data, null_blob);
        auto partition_hash = pegasus_key_hash(req);
        // TODO: set the timeout
        client->ttl(req,
                    on_ttl_reply,
                    std::chrono::milliseconds(2000),
                    0,
                    partition_hash,
                    proxy_session::hash());
    }
}

void redis_parser::geo_radius(message_entry &entry)
{
    redis_request &redis_request = entry.request;
    if (redis_request.buffers.size() < 6) {
        redis_simple_string result;
        result.is_error = true;
        result.message = "ERR wrong number of arguments for 'GEORADIUS' command";
        reply_message(entry, result);
    } else {
        std::shared_ptr<proxy_session> ref_this = shared_from_this();

        // TODO: set the timeout
        std::string hash_key = redis_request.buffers[1].data.to_string();
        double radius_m = atof(redis_request.buffers[4].data.to_string().c_str());
        std::string unit = redis_request.buffers[5].data.to_string();
        if (unit == "km") {
            radius_m *= 1000;
        } else if (unit == "mi") {
            radius_m *= 1609.344;
        } else if (unit == "ft") {
            radius_m *= 0.3048;
        } else {
            // keep as meter unit
        }
        bool WITHCOORD = false;
        bool WITHDIST = false;
        bool WITHHASH = false;
        bool WITHVALUE = false;
        geo::geo_client::SortType sort_type = geo::geo_client::SortType::random;
        int count = -1;
        for (int i = 5; i < redis_request.buffers.size(); ++i) {
            const std::string &opt = redis_request.buffers[i].data.to_string();
            if (opt == "WITHCOORD") {
                WITHCOORD = true;
            } else if (opt == "WITHDIST") {
                WITHDIST = true;
            } else if (opt == "WITHHASH") {
                WITHHASH = true;
            } else if (opt == "WITHVALUE") {
                WITHVALUE = true;
            } else if (opt == "COUNT" && i + 1 < redis_request.buffers.size()) {
                const std::string &str_count = redis_request.buffers[i + 1].data.to_string();
                if (!dsn::buf2int32(str_count, count)) {
                    derror_f("COUNT({}) opt is error, use {}", str_count, count);
                }
            } else if (opt == "ASC") {
                sort_type = geo::geo_client::SortType::asc;
            } else if (opt == "DESC") {
                sort_type = geo::geo_client::SortType::desc; // TODO
            }
        }

        auto search_callback = [ref_this, this, &entry, WITHCOORD, WITHDIST, WITHHASH, WITHVALUE](
            int ec, std::list<geo::SearchResult> &&results) {
            if (status == removed) {
                return;
            }

            if (PERR_OK != ec) {
                redis_simple_string result;
                result.is_error = true;
                result.message = std::string("ERR ") + _geo_client->get_error_string(ec);
                reply_message(entry, result);
            } else {
                redis_array result;
                result.count = (int)results.size();
                for (const auto &elem : results) {
                    if (!WITHCOORD && !WITHDIST && !WITHHASH && !WITHVALUE) {
                        result.array.push_back(
                            std::shared_ptr<redis_base_type>(new redis_bulk_string(
                                (int)elem.hash_key.size(), elem.hash_key.data())));
                    } else {
                        std::shared_ptr<redis_array> tmp(new redis_array());
                        tmp->array.push_back(std::shared_ptr<redis_base_type>(new redis_bulk_string(
                            (int)elem.hash_key.size(), elem.hash_key.data())));
                        tmp->count++;
                        // NOTE: the order of WITH* options should be not changed because of the
                        // protocol
                        if (WITHDIST) {
                            std::string dist = std::to_string(elem.distance);
                            std::shared_ptr<char> dist_buf =
                                dsn::utils::make_shared_array<char>(dist.size());
                            memcpy(dist_buf.get(), dist.data(), dist.size());
                            tmp->array.push_back(
                                std::shared_ptr<redis_base_type>(new redis_bulk_string(
                                    dsn::blob(std::move(dist_buf), (int)dist.size()))));
                            tmp->count++;
                        }
                        if (WITHHASH) {
                            // TODO
                        }
                        if (WITHCOORD) {
                            std::shared_ptr<redis_array> coord_tmp(new redis_array());
                            std::string lng = std::to_string(elem.lng_degrees);
                            std::shared_ptr<char> lng_buf =
                                dsn::utils::make_shared_array<char>(lng.size());
                            memcpy(lng_buf.get(), lng.data(), lng.size());
                            coord_tmp->array.push_back(
                                std::shared_ptr<redis_base_type>(new redis_bulk_string(
                                    dsn::blob(std::move(lng_buf), (int)lng.size()))));
                            coord_tmp->count++;

                            std::string lat = std::to_string(elem.lat_degrees);
                            std::shared_ptr<char> lat_buf =
                                dsn::utils::make_shared_array<char>(lat.size());
                            memcpy(lat_buf.get(), lat.data(), lat.size());
                            coord_tmp->array.push_back(
                                std::shared_ptr<redis_base_type>(new redis_bulk_string(
                                    dsn::blob(std::move(lat_buf), (int)lat.size()))));
                            coord_tmp->count++;

                            tmp->array.push_back(coord_tmp);
                            tmp->count++;
                        }
                        if (WITHVALUE) {
                            tmp->array.push_back(std::shared_ptr<redis_base_type>(
                                new redis_bulk_string((int)elem.value.size(), elem.value.data())));
                            tmp->count++;
                        }
                        result.array.push_back(tmp);
                    }
                }
                reply_message(entry, result);
            }
        };

        if (!hash_key.empty()) {
            // GEORADIUS key "" "" radius m|km|ft|mi
            _geo_client->async_search_radial(
                hash_key, "", radius_m, count, sort_type, 2000, search_callback);
        } else {
            // GEORADIUS "" longitude latitude radius m|km|ft|mi
            double lat_degrees = atof(redis_request.buffers[3].data.to_string().c_str());
            double lng_degrees = atof(redis_request.buffers[2].data.to_string().c_str());
            _geo_client->async_search_radial(
                lat_degrees, lng_degrees, radius_m, count, sort_type, 2000, search_callback);
        }
    }
}

void redis_parser::handle_command(std::unique_ptr<message_entry> &&entry)
{
    message_entry &e = *entry.get();
    redis_request &request = e.request;
    e.sequence_id = ++next_seqid;
    e.response = nullptr;

    {
        ::dsn::service::zauto_lock l(_rlock);
        pending_response.emplace_back(std::move(entry));
    }

    dassert(request.length > 0, "invalid request, request.length = %d", request.length);
    ::dsn::blob &command = request.buffers[0].data;
    redis_call_handler handler = redis_parser::get_handler(command.data(), command.length());
    handler(this, e);
}

void redis_parser::redis_integer::marshalling(::dsn::binary_writer &write_stream) const
{
    write_stream.write_pod(':');
    std::string result = std::to_string(value);
    write_stream.write(result.c_str(), (int)result.length());
    write_stream.write_pod(CR);
    write_stream.write_pod(LF);
}

void redis_parser::redis_simple_string::marshalling(::dsn::binary_writer &write_stream) const
{
    write_stream.write_pod(is_error ? '-' : '+');
    write_stream.write(message.c_str(), (int)message.length());
    write_stream.write_pod(CR);
    write_stream.write_pod(LF);
}

void redis_parser::redis_bulk_string::marshalling(::dsn::binary_writer &write_stream) const
{
    write_stream.write_pod('$');
    std::string result = std::to_string(length);
    write_stream.write(result.c_str(), (int)result.length());
    write_stream.write_pod(CR);
    write_stream.write_pod(LF);
    if (length < 0) {
        return;
    }
    if (length > 0) {
        dassert(data.length() == length, "%u VS %d", data.length(), length);
        write_stream.write(data.data(), length);
    }
    write_stream.write_pod(CR);
    write_stream.write_pod(LF);
}

void redis_parser::redis_array::marshalling(::dsn::binary_writer &write_stream) const
{
    write_stream.write_pod('*');
    std::string result = std::to_string(count);
    write_stream.write(result.c_str(), (int)result.length());
    write_stream.write_pod(CR);
    write_stream.write_pod(LF);
    if (count > 0) {
        dassert_f(array.size() == count, "{} VS {}", array.size(), count);
        for (auto elem : array) {
            elem->marshalling(write_stream);
        }
    }
}
}
} // namespace
