// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

#include <cctype>
#include <cerrno>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <string>
#include <type_traits>

#include <string_view>

namespace dsn {

namespace internal {

template <typename T>
bool buf2signed(std::string_view buf, T &result)
{
    static_assert(std::is_signed<T>::value, "buf2signed works only with signed integer");

    if (buf.empty()) {
        return false;
    }

    std::string str(buf.data(), buf.length());
    const int saved_errno = errno;
    errno = 0;
    char *p = nullptr;
    long long v = std::strtoll(str.data(), &p, 0);

    if (p - str.data() != str.length()) {
        return false;
    }

    if (v > std::numeric_limits<T>::max() || v < std::numeric_limits<T>::min() || errno != 0) {
        return false;
    }

    if (errno == 0) {
        errno = saved_errno;
    }

    result = v;
    return true;
}

template <typename T>
bool buf2unsigned(std::string_view buf, T &result)
{
    static_assert(std::is_unsigned<T>::value, "buf2unsigned works only with unsigned integer");

    if (buf.empty()) {
        return false;
    }

    std::string str(buf.data(), buf.length());
    const int saved_errno = errno;
    errno = 0;
    char *p = nullptr;
    unsigned long long v = std::strtoull(str.data(), &p, 0);

    if (p - str.data() != str.length()) {
        return false;
    }

    if (v > std::numeric_limits<T>::max() || v < std::numeric_limits<T>::min() || errno != 0) {
        return false;
    }

    if (errno == 0) {
        errno = saved_errno;
    }

    // strtoull() will convert a negative integer to an unsigned integer,
    // return false in this condition. (but we consider "-0" is correct)
    if (v != 0 && str.find('-') != std::string::npos) {
        return false;
    }

    result = v;
    return true;
}
} // namespace internal

/// buf2*: `result` will keep unmodified if false is returned.

inline bool buf2int32(std::string_view buf, int32_t &result)
{
    return internal::buf2signed(buf, result);
}

inline bool buf2int64(std::string_view buf, int64_t &result)
{
    return internal::buf2signed(buf, result);
}

inline bool buf2uint32(std::string_view buf, uint32_t &result)
{
    return internal::buf2unsigned(buf, result);
}

inline bool buf2uint64(std::string_view buf, uint64_t &result)
{
    return internal::buf2unsigned(buf, result);
}

inline bool buf2uint16(std::string_view buf, uint16_t &result)
{
    return internal::buf2unsigned(buf, result);
}

inline bool buf2bool(std::string_view buf, bool &result, bool ignore_case = true)
{
    std::string data(buf.data(), buf.length());
    if (ignore_case) {
        std::transform(data.begin(), data.end(), data.begin(), ::tolower);
    }
    if (data == "true") {
        result = true;
        return true;
    }
    if (data == "false") {
        result = false;
        return true;
    }
    return false;
}

inline bool buf2double(std::string_view buf, double &result)
{
    if (buf.empty()) {
        return false;
    }

    std::string str(buf.data(), buf.length());
    const int saved_errno = errno;
    errno = 0;
    char *p = nullptr;
    double v = std::strtod(str.data(), &p);

    if (p - str.data() != str.length()) {
        return false;
    }

    if (v == HUGE_VAL || v == -HUGE_VAL || std::isnan(v) || errno != 0) {
        return false;
    }

    if (errno == 0) {
        errno = saved_errno;
    }

    result = v;
    return true;
}

#define DEF_BUF2NUMERIC_FUNC(type, postfix)                                                        \
    inline bool buf2numeric(std::string_view buf, type &result)                                    \
    {                                                                                              \
        return buf2##postfix(buf, result);                                                         \
    }

#define DEF_BUF2INT_FUNC(type) DEF_BUF2NUMERIC_FUNC(type##_t, type)

DEF_BUF2INT_FUNC(int32)
DEF_BUF2INT_FUNC(int64)
DEF_BUF2INT_FUNC(uint16)
DEF_BUF2INT_FUNC(uint32)
DEF_BUF2INT_FUNC(uint64)

#undef DEF_BUF2INT_FUNC

#define DEF_BUF2FLOAT_FUNC(type) DEF_BUF2NUMERIC_FUNC(type, type)

DEF_BUF2FLOAT_FUNC(double)

#undef DEF_BUF2FLOAT_FUNC

#undef DEF_BUF2NUMERIC_FUNC

} // namespace dsn
