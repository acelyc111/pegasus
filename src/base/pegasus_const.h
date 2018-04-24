// Copyright (c) 2017, Xiaomi, Inc.  All rights reserved.
// This source code is licensed under the Apache License Version 2.0, which
// can be found in the LICENSE file in the root directory of this source tree.

#pragma once

#include <string>

namespace pegasus {

const int SCAN_CONTEXT_ID_VALID_MIN = 0;
const int SCAN_CONTEXT_ID_COMPLETED = -1;
const int SCAN_CONTEXT_ID_NOT_EXIST = -2;

extern const std::string ROCKSDB_ENV_RESTORE_FORCE_RESTORE;
extern const std::string ROCKSDB_ENV_RESTORE_POLICY_NAME;
extern const std::string ROCKSDB_ENV_RESTORE_BACKUP_ID;

extern const std::string ROCKSDB_ENV_USAGE_SCENARIO_KEY;
extern const std::string ROCKSDB_ENV_USAGE_SCENARIO_NORMAL;
extern const std::string ROCKSDB_ENV_USAGE_SCENARIO_PREFER_WRITE;
extern const std::string ROCKSDB_ENV_USAGE_SCENARIO_BULK_LOAD;

extern const std::string ROCKSDB_MANUAL_COMPACT_BOTTOMMOST_LEVEL_COMPACTION_KEY;
extern const std::string ROCKSDB_MANUAL_COMPACT_BOTTOMMOST_LEVEL_COMPACTION_FORCE;
extern const std::string ROCKSDB_MANUAL_COMPACT_BOTTOMMOST_LEVEL_COMPACTION_SKIP;

extern const std::string ROCKSDB_MANUAL_COMPACT_TARGET_LEVEL_KEY;

} // namespace
