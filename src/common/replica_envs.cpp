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

#include "common/replica_envs.h"

using std::string;

namespace dsn {

const string replica_envs::DENY_CLIENT_REQUEST("replica.deny_client_request");
const string replica_envs::WRITE_QPS_THROTTLING("replica.write_throttling");
const string replica_envs::WRITE_SIZE_THROTTLING("replica.write_throttling_by_size");
const uint64_t replica_envs::MIN_SLOW_QUERY_THRESHOLD_MS = 20;
const string replica_envs::SLOW_QUERY_THRESHOLD("replica.slow_query_threshold");
const string replica_envs::ROCKSDB_USAGE_SCENARIO("rocksdb.usage_scenario");
const string replica_envs::TABLE_LEVEL_DEFAULT_TTL("default_ttl");
const string MANUAL_COMPACT_PREFIX("manual_compact.");
const string replica_envs::MANUAL_COMPACT_DISABLED(MANUAL_COMPACT_PREFIX + "disabled");
const string replica_envs::MANUAL_COMPACT_MAX_CONCURRENT_RUNNING_COUNT(
    MANUAL_COMPACT_PREFIX + "max_concurrent_running_count");
const string MANUAL_COMPACT_ONCE_PREFIX(MANUAL_COMPACT_PREFIX + "once.");
const string replica_envs::MANUAL_COMPACT_ONCE_TRIGGER_TIME(MANUAL_COMPACT_ONCE_PREFIX +
                                                            "trigger_time");
const string replica_envs::MANUAL_COMPACT_ONCE_TARGET_LEVEL(MANUAL_COMPACT_ONCE_PREFIX +
                                                            "target_level");
const string replica_envs::MANUAL_COMPACT_ONCE_BOTTOMMOST_LEVEL_COMPACTION(
    MANUAL_COMPACT_ONCE_PREFIX + "bottommost_level_compaction");
const string MANUAL_COMPACT_PERIODIC_PREFIX(MANUAL_COMPACT_PREFIX + "periodic.");
const string replica_envs::MANUAL_COMPACT_PERIODIC_TRIGGER_TIME(MANUAL_COMPACT_PERIODIC_PREFIX +
                                                                "trigger_time");
const string replica_envs::MANUAL_COMPACT_PERIODIC_TARGET_LEVEL(MANUAL_COMPACT_PERIODIC_PREFIX +
                                                                "target_level");
const string replica_envs::MANUAL_COMPACT_PERIODIC_BOTTOMMOST_LEVEL_COMPACTION(
    MANUAL_COMPACT_PERIODIC_PREFIX + "bottommost_level_compaction");
const string
    replica_envs::ROCKSDB_CHECKPOINT_RESERVE_MIN_COUNT("rocksdb.checkpoint.reserve_min_count");
const string replica_envs::ROCKSDB_CHECKPOINT_RESERVE_TIME_SECONDS(
    "rocksdb.checkpoint.reserve_time_seconds");
const string replica_envs::ROCKSDB_ITERATION_THRESHOLD_TIME_MS(
    "replica.rocksdb_iteration_threshold_time_ms");
const string replica_envs::ROCKSDB_BLOCK_CACHE_ENABLED("replica.rocksdb_block_cache_enabled");
const string replica_envs::BUSINESS_INFO("business.info");
const string replica_envs::REPLICA_ACCESS_CONTROLLER_ALLOWED_USERS(
    "replica_access_controller.allowed_users");
const string replica_envs::READ_QPS_THROTTLING("replica.read_throttling");
const string replica_envs::READ_SIZE_THROTTLING("replica.read_throttling_by_size");
const string replica_envs::SPLIT_VALIDATE_PARTITION_HASH("replica.split.validate_partition_hash");
const string replica_envs::USER_SPECIFIED_COMPACTION("user_specified_compaction");
const string replica_envs::BACKUP_REQUEST_QPS_THROTTLING("replica.backup_request_throttling");
const string replica_envs::ROCKSDB_ALLOW_INGEST_BEHIND("rocksdb.allow_ingest_behind");
const string replica_envs::UPDATE_MAX_REPLICA_COUNT("max_replica_count.update");

} // namespace dsn
