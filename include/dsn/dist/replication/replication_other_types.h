/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Microsoft Corporation
 * 
 * -=- Robust Distributed System Nucleus (rDSN) -=- 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Description:
 *     What is this file about?
 *
 * Revision history:
 *     xxxx-xx-xx, author, first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */

/**
 * Autogenerated by Thrift Compiler (@PACKAGE_VERSION@)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef replication_OTHER_TYPES_H
#define replication_OTHER_TYPES_H

# include <dsn/cpp/autoref_ptr.h>
# include <dsn/dist/replication/replication.types.h>
# include <sstream>
# include <dsn/cpp/json_helper.h>
# include <dsn/internal/enum_helper.h>

namespace dsn {
    namespace replication {

        typedef int32_t get_app_id();
        typedef int64_t ballot;
        typedef int64_t decree;

        #define invalid_ballot ((::dsn::replication::ballot)-1LL)
        #define invalid_decree ((::dsn::replication::decree)-1LL)
        #define invalid_offset (-1LL)
        #define invalid_signature 0

        class replica;
        typedef dsn::ref_ptr<replica> replica_ptr;
                    
        class replica_stub;
        typedef dsn::ref_ptr<replica_stub> replica_stub_ptr;

        class mutation;
        typedef dsn::ref_ptr<mutation> mutation_ptr;

        class mutation_log;
        typedef dsn::ref_ptr<mutation_log> mutation_log_ptr;

         ENUM_BEGIN2(partition_status::type, partition_status,  partition_status::PS_INVALID)
            ENUM_REG(partition_status::PS_INACTIVE)
            ENUM_REG(partition_status::PS_ERROR)
            ENUM_REG(partition_status::PS_PRIMARY)
            ENUM_REG(partition_status::PS_SECONDARY)
            ENUM_REG(partition_status::PS_POTENTIAL_SECONDARY)
        ENUM_END2(partition_status::type, partition_status)

        ENUM_BEGIN2(read_semantic::type, read_semantic, read_semantic::ReadInvalid)
            ENUM_REG(read_semantic::ReadLastUpdate)
            ENUM_REG(read_semantic::ReadOutdated)
            ENUM_REG(read_semantic::ReadSnapshot)
        ENUM_END2(read_semantic::type, read_semantic)

        ENUM_BEGIN2(learn_type::type, learn_type, learn_type::LT_INVALID)
            ENUM_REG(learn_type::LT_CACHE)
            ENUM_REG(learn_type::LT_APP)
            ENUM_REG(learn_type::LT_LOG)
        ENUM_END2(learn_type::type, learn_type)

        ENUM_BEGIN2(learner_status::type, learner_status, learner_status::LearningInvalid)
            ENUM_REG(learner_status::LearningWithoutPrepare)
            ENUM_REG(learner_status::LearningWithPrepareTransient)
            ENUM_REG(learner_status::LearningWithPrepare)
            ENUM_REG(learner_status::LearningSucceeded)
            ENUM_REG(learner_status::LearningFailed)
        ENUM_END2(learner_status::type, learner_status)

        ENUM_BEGIN2(config_type::type, config_type, config_type::CT_INVALID)
            ENUM_REG(config_type::CT_ASSIGN_PRIMARY)
            ENUM_REG(config_type::CT_UPGRADE_TO_PRIMARY)
            ENUM_REG(config_type::CT_ADD_SECONDARY)
            ENUM_REG(config_type::CT_UPGRADE_TO_SECONDARY)
            ENUM_REG(config_type::CT_DOWNGRADE_TO_SECONDARY)
            ENUM_REG(config_type::CT_DOWNGRADE_TO_INACTIVE)
            ENUM_REG(config_type::CT_REMOVE)
            ENUM_REG(config_type::CT_ADD_SECONDARY_FOR_LB)
        ENUM_END2(config_type::type, config_type)

        ENUM_BEGIN2(app_status::type, app_status, app_status::AS_INVALID)
            ENUM_REG(app_status::AS_AVAILABLE)
            ENUM_REG(app_status::AS_CREATING)
            ENUM_REG(app_status::AS_CREATE_FAILED)
            ENUM_REG(app_status::AS_DROPPING)
            ENUM_REG(app_status::AS_DROP_FAILED)
            ENUM_REG(app_status::AS_DROPPED)
        ENUM_END2(app_status::type, app_status)

        ENUM_BEGIN2(node_status::type, node_status, node_status::NS_INVALID)
            ENUM_REG(node_status::NS_ALIVE)
            ENUM_REG(node_status::NS_UNALIVE)
        ENUM_END2(node_status::type, node_status)

        ENUM_BEGIN2(balancer_type::type, balancer_type, balancer_type::BT_INVALID)
            ENUM_REG(balancer_type::BT_MOVE_PRIMARY)
            ENUM_REG(balancer_type::BT_COPY_PRIMARY)
            ENUM_REG(balancer_type::BT_COPY_SECONDARY)
        ENUM_END2(balancer_type::type, balancer_type)
/*
        inline void json_encode(std::stringstream& out, const partition_configuration& config)
        {
            JSON_DICT_ENTRIES(out, config, app_type, envs, gpid, ballot, max_replica_count, primary, secondaries, last_drops, last_committed_decree);
        }
        inline void json_encode(std::stringstream& out, const partition_status::type& status)
        {
            json_encode(out, enum_to_string(status));
        }
        inline void json_encode(std::stringstream& out, const replica_configuration& config)
        {
            JSON_DICT_ENTRIES(out, config, gpid, ballot, primary, status, learner_signature);
        }
        
        inline void json_encode(std::stringstream& out, const gpid& gpid)
        {
            JSON_DICT_ENTRIES(out, gpid, app_id, partition_index);
        }

        inline void json_encode(std::stringstream& out, const app_info& config)
        {
            JSON_DICT_ENTRIES(out, config, status, app_type, app_name, app_id, partition_count, envs, is_stateful);
        }

        inline void json_encode(std::stringstream& out, const node_info& config)
        {
            JSON_DICT_ENTRIES(out, config, status, address);
        }

        inline void json_encode(std::stringstream& out, const app_status::type& status)
        {
            out << "\"" << enum_to_string(status) << "\"";
        }*/

        inline bool is_partition_config_equal(const partition_configuration& pc1, const partition_configuration& pc2)
        {
            // last_drops is not considered into equality check
            return pc1.ballot == pc2.ballot &&
                   pc1.pid == pc2.pid &&
                   pc1.max_replica_count == pc2.max_replica_count &&
                   pc1.primary == pc2.primary &&
                   pc1.secondaries == pc2.secondaries &&
                   pc1.last_committed_decree == pc2.last_committed_decree;
        }
        
        class replica_helper
        {
        public:
            static bool remove_node(::dsn::rpc_address node, /*inout*/ std::vector< ::dsn::rpc_address>& nodeList);
            static bool get_replica_config(const partition_configuration& partition_config, ::dsn::rpc_address node, /*out*/ replica_configuration& replica_config);
            static void load_meta_servers(/*out*/ std::vector<dsn::rpc_address>& servers, const char* section = "meta_server", const char* key = "server_list");
        };
    }

    inline void json_encode(std::stringstream& out, const dsn::rpc_address& address)
    {
        out << "\"" << address.to_string() << "\"";
    }
} // namespace

#endif
