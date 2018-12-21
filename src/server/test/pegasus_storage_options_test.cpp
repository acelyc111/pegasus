// Copyright (c) 2017, Xiaomi, Inc.  All rights reserved.
// This source code is licensed under the Apache License Version 2.0, which
// can be found in the LICENSE file in the root directory of this source tree.

#include "pegasus_server_test_base.h"

namespace pegasus {
namespace server {

auto none = rocksdb::kNoCompression;
auto snappy = rocksdb::kSnappyCompression;
auto lz4 = rocksdb::kLZ4Compression;
auto zstd = rocksdb::kZSTD;

class pegasus_storage_options_test : public pegasus_server_test_base
{
public:
    std::string compression_header;

    pegasus_storage_options_test()
    {
        _server->_db_opts.num_levels = 7;
        compression_header = _server->compression_header;
    }

    void compression_type_convert(const std::string &str, rocksdb::CompressionType type)
    {
        rocksdb::CompressionType tmp_type;
        ASSERT_TRUE(_server->compression_str2type(str, tmp_type)) << str;
        ASSERT_EQ(tmp_type, type) << str << " vs. " << type;

        ASSERT_EQ(str, _server->compression_type2str(type)) << str << " vs. " << type;
    }

    void
    compression_types_convert_ok(const std::string &config,
                                 const std::vector<rocksdb::CompressionType> &compression_per_level)
    {
        std::vector<rocksdb::CompressionType> tmp_compression_per_level;
        ASSERT_TRUE(_server->parse_compression_types(config, tmp_compression_per_level)) << config;
        ASSERT_EQ(tmp_compression_per_level, compression_per_level) << config;
    }

    void compression_types_convert_fail(const std::string &config)
    {
        static const std::vector<rocksdb::CompressionType> &old_compression_per_level = {
            none, lz4, snappy, zstd, lz4, snappy, zstd};
        std::vector<rocksdb::CompressionType> tmp_compression_per_level = old_compression_per_level;
        ASSERT_FALSE(_server->parse_compression_types(config, tmp_compression_per_level)) << config;
        ASSERT_EQ(tmp_compression_per_level, old_compression_per_level) << config;
    }

    bool compression_str2type(const std::string &compression_str, rocksdb::CompressionType &type)
    {
        return _server->compression_str2type(compression_str, type);
    }

    std::string compression_type2str(rocksdb::CompressionType type)
    {
        return _server->compression_type2str(type);
    }
};

TEST_F(pegasus_storage_options_test, compression_type_convert_ok)
{
    compression_type_convert("none", none);
    compression_type_convert("snappy", snappy);
    compression_type_convert("lz4", lz4);
    compression_type_convert("zstd", zstd);
}

TEST_F(pegasus_storage_options_test, compression_type_convert_not_support)
{
    rocksdb::CompressionType tmp_type;
    ASSERT_FALSE(compression_str2type("not_support_zip", tmp_type));

    ASSERT_EQ("", compression_type2str(rocksdb::kZlibCompression));
    ASSERT_EQ("", compression_type2str(rocksdb::kBZip2Compression));
    ASSERT_EQ("", compression_type2str(rocksdb::kLZ4HCCompression));
    ASSERT_EQ("", compression_type2str(rocksdb::kXpressCompression));
    ASSERT_EQ("", compression_type2str(rocksdb::kZSTDNotFinalCompression));
    ASSERT_EQ("", compression_type2str(rocksdb::kDisableCompressionOption));
}

TEST_F(pegasus_storage_options_test, compression_types_convert_ok)
{
    // Old style.
    compression_types_convert_ok("none", {none, none, none, none, none, none, none});
    compression_types_convert_ok("snappy", {none, none, snappy, snappy, snappy, snappy, snappy});
    compression_types_convert_ok("lz4", {none, none, lz4, lz4, lz4, lz4, lz4});
    compression_types_convert_ok("zstd", {none, none, zstd, zstd, zstd, zstd, zstd});

    // New style.
    compression_types_convert_ok(compression_header + "none",
                                 {none, none, none, none, none, none, none});
    compression_types_convert_ok(compression_header + "none,snappy",
                                 {none, snappy, snappy, snappy, snappy, snappy, snappy});
    compression_types_convert_ok(compression_header + "none,lz4,snappy,zstd",
                                 {none, lz4, snappy, zstd, zstd, zstd, zstd});
    compression_types_convert_ok(compression_header + "none,lz4,snappy,zstd,lz4,snappy,zstd",
                                 {none, lz4, snappy, zstd, lz4, snappy, zstd});
    compression_types_convert_ok(compression_header + "none,lz4,snappy,zstd,lz4,snappy,zstd,zstd",
                                 {none, lz4, snappy, zstd, lz4, snappy, zstd});
}

TEST_F(pegasus_storage_options_test, compression_types_convert_fail)
{
    // Old style.
    compression_types_convert_fail("none1");
    compression_types_convert_fail("Snappy");
    compression_types_convert_fail(",zstd");

    // New style.
    compression_types_convert_fail(compression_header + ":snappy");
    compression_types_convert_fail(compression_header + "snappy,snappy1");
    compression_types_convert_fail("per_leve:snappy");
    compression_types_convert_fail("per_levelsnappy");
}

TEST_F(pegasus_storage_options_test, check_rocksdb_compression_types) {}

} // namespace server
} // namespace pegasus
