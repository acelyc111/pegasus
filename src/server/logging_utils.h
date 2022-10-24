/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include "utils/fmt_logging.h"

/// Utilities for logging the operation on rocksdb.

#define derror_rocksdb(op, error, ...)                                                             \
    LOG_ERROR_F("{}: rocksdb {} failed: error = {} [{}]",                                          \
                replica_name(),                                                                    \
                op,                                                                                \
                error,                                                                             \
                fmt::format(__VA_ARGS__))

#define ddebug_rocksdb(op, ...)                                                                    \
    LOG_INFO_F("{}: rocksdb {}: [{}]", replica_name(), op, fmt::format(__VA_ARGS__))

#define dwarn_rocksdb(op, ...)                                                                     \
    LOG_WARNING_F("{}: rocksdb {}: [{}]", replica_name(), op, fmt::format(__VA_ARGS__))
