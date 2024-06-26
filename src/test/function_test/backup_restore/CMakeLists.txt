# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

set(MY_PROJ_NAME backup_restore_test)
project(${MY_PROJ_NAME} C CXX)
set(MY_PROJ_SRC "")
set(MY_SRC_SEARCH_MODE "GLOB")

set(MY_PROJ_LIBS
        dsn_client
        dsn_replication_common
        dsn_utils
        gtest
        sasl2
        gssapi_krb5
        krb5
        function_test_utils
        pegasus_client_static
        test_utils
        rocksdb
        lz4
        zstd
        snappy)

set(MY_BOOST_LIBS Boost::system Boost::filesystem)

set(MY_BINPLACES "../config.ini" "../run.sh")

dsn_add_test()
