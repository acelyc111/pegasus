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

#include "base/test/base_test_utils.h"

#include <memory>
#include <fmt/format.h>
#include <rocksdb/slice.h>

namespace pegasus {
std::string generate_value(value_schema *schema,
                           uint32_t expire_ts,
                           uint64_t time_tag,
                           std::string_view user_data)
{
    std::string write_buf;
    std::vector<rocksdb::Slice> write_slices;
    value_params params{write_buf, write_slices};
    params.fields[value_field_type::EXPIRE_TIMESTAMP] =
        std::make_unique<expire_timestamp_field>(expire_ts);
    params.fields[value_field_type::TIME_TAG] = std::make_unique<time_tag_field>(time_tag);
    params.fields[value_field_type::USER_DATA] = std::make_unique<user_data_field>(user_data);

    rocksdb::SliceParts sparts = schema->generate_value(params);
    std::string raw_value;
    for (int i = 0; i < sparts.num_parts; i++) {
        raw_value.append(sparts.parts[i].ToString(false /*hex*/));
    }
    return raw_value;
}
} // namespace pegasus
