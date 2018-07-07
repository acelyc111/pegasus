// Copyright (c) 2018-present, Xiaomi, Inc.  All rights reserved.
// This source code is licensed under the Apache License Version 2.0, which
// can be found in the LICENSE file in the root directory of this source tree.

#include <dsn/utility/string_conv.h>
#include "latlng_extractor.h"

namespace pegasus {
namespace geo {

#define white_space(c) ((c) == ' ' || (c) == '\t')
#define valid_digit(c) ((c) >= '0' && (c) <= '9')

template <typename T>
bool naive(T &r, const char *p)
{
    // Skip leading white space, if any.
    while (white_space(*p)) {
        p += 1;
    }

    r = 0.0;
    int c = 0; // counter to check how many numbers we got!

    // Get the sign!
    bool neg = false;
    if (*p == '-') {
        neg = true;
        ++p;
    } else if (*p == '+') {
        neg = false;
        ++p;
    }

    // Get the digits before decimal point
    while (valid_digit(*p)) {
        r = (r * 10.0) + (*p - '0');
        ++p;
        ++c;
    }

    // Get the digits after decimal point
    if (*p == '.') {
        T f = 0.0;
        T scale = 1.0;
        ++p;
        while (*p >= '0' && *p <= '9') {
            f = (f * 10.0) + (*p - '0');
            ++p;
            scale *= 10.0;
            ++c;
        }
        r += f / scale;
    }

    // FIRST CHECK:
    if (c == 0) {
        return false;
    } // we got no dezimal places! this cannot be any number!

    // Get the digits after the "e"/"E" (exponenet)
    if (*p == 'e' || *p == 'E') {
        unsigned int e = 0;

        bool negE = false;
        ++p;
        if (*p == '-') {
            negE = true;
            ++p;
        } else if (*p == '+') {
            negE = false;
            ++p;
        }
        // Get exponent
        c = 0;
        while (valid_digit(*p)) {
            e = (e * 10) + (*p - '0');
            ++p;
            ++c;
        }
        if (!neg && e > std::numeric_limits<T>::max_exponent10) {
            e = std::numeric_limits<T>::max_exponent10;
        } else if (e < std::numeric_limits<T>::min_exponent10) {
            e = std::numeric_limits<T>::max_exponent10;
        }
        // SECOND CHECK:
        if (c == 0) {
            return false;
        } // we got no  exponent! this was not intended!!

        T scaleE = 1.0;
        // Calculate scaling factor.

        while (e >= 50) {
            scaleE *= 1E50;
            e -= 50;
        }
        // while (e >=  8) { scaleE *= 1E8;  e -=  8; }
        while (e > 0) {
            scaleE *= 10.0;
            e -= 1;
        }

        if (negE) {
            r /= scaleE;
        } else {
            r *= scaleE;
        }
    }

    // POST CHECK:
    // skip post whitespaces
    while (white_space(*p)) {
        ++p;
    }
    if (*p != '\0') {
        return false;
    } // if next character is not the terminating character

    // Apply sign to number
    if (neg) {
        r = -r;
    }

    return true;
}

void extract_indexs(const std::string &text,
                    const std::vector<int> &indexs,
                    std::vector<std::string> &values,
                    char splitter)
{
    size_t begin_pos = 0;
    size_t end_pos = 0;
    int cur_index = -1;
    for (auto index : indexs) {
        while (cur_index < index) {
            begin_pos = (cur_index == -1 ? 0 : end_pos + 1); // at first time, seek from 0
                                                             // then, seek from end_pos + 1
            end_pos = text.find(splitter, begin_pos);
            if (end_pos == std::string::npos) {
                break;
            }
            cur_index++;
        }

        if (end_pos == std::string::npos) {
            values.emplace_back(text.substr(begin_pos));
            break;
        } else {
            values.emplace_back(text.substr(begin_pos, end_pos - begin_pos));
        }
    }
}

const char *latlng_extractor_for_lbs::name() const { return "latlng_extractor_for_lbs"; }

const char *latlng_extractor_for_lbs::value_sample() const
{
    return "00:00:00:00:01:5e|2018-04-26|2018-04-28|ezp8xchrr|160.356396|39.469644|24.0|4.15|0|-1";
}

bool latlng_extractor_for_lbs::extract_from_value(const std::string &value, S2LatLng &latlng) const
{
    std::vector<std::string> data;
    extract_indexs(value, {4, 5}, data, '|');
    if (data.size() != 2) {
        return false;
    }

    std::string lng = data[0];
    std::string lat = data[1];
    double lat_degrees, lng_degrees = 0.0;
    if (!naive(lat_degrees, lat.c_str()) || !naive(lng_degrees, lng.c_str())) {
        return false;
    }
    latlng = S2LatLng::FromDegrees(lat_degrees, lng_degrees);

    return latlng.is_valid();
}

} // namespace geo
} // namespace pegasus
