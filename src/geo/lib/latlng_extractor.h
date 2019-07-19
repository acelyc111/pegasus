// Copyright (c) 2018-present, Xiaomi, Inc.  All rights reserved.
// This source code is licensed under the Apache License Version 2.0, which
// can be found in the LICENSE file in the root directory of this source tree.

#pragma once

#include <string>
#include <vector>
#include <s2/s2latlng.h>
#include <dsn/utility/strings.h>

namespace dsn {
class error_s;
} // namespace dsn

namespace pegasus {
namespace geo {

class latlng_extractor
{
public:
    // Extract latitude and longitude from value.
    bool extract_from_value(const std::string &value, S2LatLng &latlng);

    // Set latitude and longitude indices in string type value, indices are the ones
    // when the string type value split into list by '|'.
    // indices: <latitude index, longitude index>
    dsn::error_s set_latlng_indices(std::pair<uint32_t, uint32_t> indices);

private:
    // <latitude index, longitude index>
    std::vector<int> _sorted_indices;
    // Whether latitude and longitude indices are reversed in '_sorted_indices'.
    bool _latlng_reversed = false;
};

} // namespace geo
} // namespace pegasus
