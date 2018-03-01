// Copyright (c) 2017, Xiaomi, Inc.  All rights reserved.
// This source code is licensed under the Apache License Version 2.0, which
// can be found in the LICENSE file in the root directory of this source tree.

#pragma once

#include <stdint.h>
#include <string.h>
#include <dsn/utility/ports.h>
#include <dsn/utility/utils.h>

namespace pegasus {

// =====================================================================================
// rocksdb key = [hash_key_len(uint16_t)] [hash_key(bytes)] [sort_key(bytes)]

// generate rocksdb key.
// T may be std::string or ::dsn::blob.
// data is copied into 'key'.
template <typename T>
void pegasus_generate_key(::dsn::blob &key, const T &hash_key, const T &sort_key)
{
    dassert(hash_key.length() < UINT16_MAX, "hash key length must be less than UINT16_MAX");

    int len = 2 + hash_key.length() + sort_key.length();
    std::shared_ptr<char> buf(::dsn::make_shared_array<char>(len));

    // hash_key_len is in big endian
    uint16_t hash_key_len = hash_key.length();
    *((int16_t *)buf.get()) = htobe16((int16_t)hash_key_len);

    ::memcpy(buf.get() + 2, hash_key.data(), hash_key_len);

    if (sort_key.length() > 0) {
        ::memcpy(buf.get() + 2 + hash_key_len, sort_key.data(), sort_key.length());
    }

    key.assign(std::move(buf), 0, len);
}

// generate the adjacent next rocksdb key for scan.
// T may be std::string or ::dsn::blob.
// data is copied into 'key'.
template <typename T>
void pegasus_generate_next_blob(::dsn::blob &key, const T &hash_key)
{
    dassert(hash_key.length() < UINT16_MAX, "hash key length must be less than UINT16_MAX");

    int hash_key_len = hash_key.length();
    std::shared_ptr<char> buf(::dsn::make_shared_array<char>(hash_key_len + 2));

    *((int16_t *)buf.get()) = htobe16((int16_t)hash_key_len);
    ::memcpy(buf.get() + 2, hash_key.data(), hash_key_len);

    //the next key:
    //1.find the last valid char
    char *p = buf.get() + hash_key_len + 1;
    while (*p == 0xFF)
        p--;
    //2.plus number 1
    (*p)++;

    //3.update length
    int len = p - buf.get() + 1;
    key.assign(std::move(buf), 0, len);
}

// restore hash_key and sort_key from rocksdb value.
// no data copied.
inline void
pegasus_restore_key(const ::dsn::blob &key, ::dsn::blob &hash_key, ::dsn::blob &sort_key)
{
    dassert(key.length() >= 2, "key length must be no less than 2");

    // hash_key_len is in big endian
    uint16_t hash_key_len = be16toh(*(int16_t *)(key.data()));

    if (hash_key_len > 0) {
        dassert(key.length() >= 2 + hash_key_len,
                "key length must be no less than (2 + hash_key_len)");
        hash_key = key.range(2, hash_key_len);
    } else {
        hash_key = ::dsn::blob();
    }

    if (key.length() > 2 + hash_key_len) {
        sort_key = key.range(2 + hash_key_len);
    } else {
        sort_key = ::dsn::blob();
    }
}

// restore hash_key and sort_key from rocksdb value.
// data is copied into output 'hash_key' and 'sort_key'.
inline void
pegasus_restore_key(const ::dsn::blob &key, std::string &hash_key, std::string &sort_key)
{
    dassert(key.length() >= 2, "key length must be no less than 2");

    // hash_key_len is in big endian
    uint16_t hash_key_len = be16toh(*(int16_t *)(key.data()));

    if (hash_key_len > 0) {
        dassert(key.length() >= 2 + hash_key_len,
                "key length must be no less than (2 + hash_key_len)");
        hash_key.assign(key.data() + 2, hash_key_len);
    } else {
        hash_key.clear();
    }

    if (key.length() > 2 + hash_key_len) {
        sort_key.assign(key.data() + 2 + hash_key_len, key.length() - 2 - hash_key_len);
    } else {
        sort_key.clear();
    }
}

// calculate hash from rocksdb key.
inline uint64_t pegasus_key_hash(const ::dsn::blob &key)
{
    dassert(key.length() >= 2, "key length must be no less than 2");

    // hash_key_len is in big endian
    uint16_t hash_key_len = be16toh(*(int16_t *)(key.data()));

    if (hash_key_len > 0) {
        // hash_key_len > 0, compute hash from hash_key
        dassert(key.length() >= 2 + hash_key_len,
                "key length must be no less than (2 + hash_key_len)");
        return dsn_crc64_compute(key.buffer_ptr() + 2, hash_key_len, 0);
    } else {
        // hash_key_len == 0, compute hash from sort_key
        return dsn_crc64_compute(key.buffer_ptr() + 2, key.length() - 2, 0);
    }
}

} // namespace
