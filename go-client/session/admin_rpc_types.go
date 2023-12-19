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
// Code generated by "generator -i=admin.csv > admin_rpc_types.go"; DO NOT EDIT.
package session

import (
	"context"
	"fmt"

	"github.com/apache/incubator-pegasus/go-client/idl/admin"
	"github.com/apache/incubator-pegasus/go-client/idl/base"
)

func (ms *metaSession) dropApp(ctx context.Context, req *admin.ConfigurationDropAppRequest) (*admin.ConfigurationDropAppResponse, error) {
	arg := admin.NewAdminClientDropAppArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_DROP_APP")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientDropAppResult)
	return ret.GetSuccess(), nil
}

// DropApp is auto-generated
func (m *MetaManager) DropApp(ctx context.Context, req *admin.ConfigurationDropAppRequest) (*admin.ConfigurationDropAppResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.dropApp(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.ConfigurationDropAppResponse), fmt.Errorf("DropApp failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.ConfigurationDropAppResponse), nil
	}
	return nil, err
}

func (ms *metaSession) createApp(ctx context.Context, req *admin.ConfigurationCreateAppRequest) (*admin.ConfigurationCreateAppResponse, error) {
	arg := admin.NewAdminClientCreateAppArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_CREATE_APP")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientCreateAppResult)
	return ret.GetSuccess(), nil
}

// CreateApp is auto-generated
func (m *MetaManager) CreateApp(ctx context.Context, req *admin.ConfigurationCreateAppRequest) (*admin.ConfigurationCreateAppResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.createApp(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.ConfigurationCreateAppResponse), fmt.Errorf("CreateApp failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.ConfigurationCreateAppResponse), nil
	}
	return nil, err
}

func (ms *metaSession) recallApp(ctx context.Context, req *admin.ConfigurationRecallAppRequest) (*admin.ConfigurationRecallAppResponse, error) {
	arg := admin.NewAdminClientRecallAppArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_RECALL_APP")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientRecallAppResult)
	return ret.GetSuccess(), nil
}

// RecallApp is auto-generated
func (m *MetaManager) RecallApp(ctx context.Context, req *admin.ConfigurationRecallAppRequest) (*admin.ConfigurationRecallAppResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.recallApp(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.ConfigurationRecallAppResponse), fmt.Errorf("RecallApp failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.ConfigurationRecallAppResponse), nil
	}
	return nil, err
}

func (ms *metaSession) listApps(ctx context.Context, req *admin.ConfigurationListAppsRequest) (*admin.ConfigurationListAppsResponse, error) {
	arg := admin.NewAdminClientListAppsArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_LIST_APPS")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientListAppsResult)
	return ret.GetSuccess(), nil
}

// ListApps is auto-generated
func (m *MetaManager) ListApps(ctx context.Context, req *admin.ConfigurationListAppsRequest) (*admin.ConfigurationListAppsResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.listApps(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.ConfigurationListAppsResponse), fmt.Errorf("ListApps failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.ConfigurationListAppsResponse), nil
	}
	return nil, err
}

func (ms *metaSession) queryDuplication(ctx context.Context, req *admin.DuplicationQueryRequest) (*admin.DuplicationQueryResponse, error) {
	arg := admin.NewAdminClientQueryDuplicationArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_QUERY_DUPLICATION")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientQueryDuplicationResult)
	return ret.GetSuccess(), nil
}

// QueryDuplication is auto-generated
func (m *MetaManager) QueryDuplication(ctx context.Context, req *admin.DuplicationQueryRequest) (*admin.DuplicationQueryResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.queryDuplication(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.DuplicationQueryResponse), fmt.Errorf("QueryDuplication failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.DuplicationQueryResponse), nil
	}
	return nil, err
}

func (ms *metaSession) modifyDuplication(ctx context.Context, req *admin.DuplicationModifyRequest) (*admin.DuplicationModifyResponse, error) {
	arg := admin.NewAdminClientModifyDuplicationArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_MODIFY_DUPLICATION")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientModifyDuplicationResult)
	return ret.GetSuccess(), nil
}

// ModifyDuplication is auto-generated
func (m *MetaManager) ModifyDuplication(ctx context.Context, req *admin.DuplicationModifyRequest) (*admin.DuplicationModifyResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.modifyDuplication(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.DuplicationModifyResponse), fmt.Errorf("ModifyDuplication failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.DuplicationModifyResponse), nil
	}
	return nil, err
}

func (ms *metaSession) addDuplication(ctx context.Context, req *admin.DuplicationAddRequest) (*admin.DuplicationAddResponse, error) {
	arg := admin.NewAdminClientAddDuplicationArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_ADD_DUPLICATION")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientAddDuplicationResult)
	return ret.GetSuccess(), nil
}

// AddDuplication is auto-generated
func (m *MetaManager) AddDuplication(ctx context.Context, req *admin.DuplicationAddRequest) (*admin.DuplicationAddResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.addDuplication(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.DuplicationAddResponse), fmt.Errorf("AddDuplication failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.DuplicationAddResponse), nil
	}
	return nil, err
}

func (ms *metaSession) queryAppInfo(ctx context.Context, req *admin.QueryAppInfoRequest) (*admin.QueryAppInfoResponse, error) {
	arg := admin.NewAdminClientQueryAppInfoArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_QUERY_APP_INFO")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientQueryAppInfoResult)
	return ret.GetSuccess(), nil
}

// QueryAppInfo is auto-generated
func (m *MetaManager) QueryAppInfo(ctx context.Context, req *admin.QueryAppInfoRequest) (*admin.QueryAppInfoResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.queryAppInfo(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.QueryAppInfoResponse), fmt.Errorf("QueryAppInfo failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.QueryAppInfoResponse), nil
	}
	return nil, err
}

func (ms *metaSession) updateAppEnv(ctx context.Context, req *admin.ConfigurationUpdateAppEnvRequest) (*admin.ConfigurationUpdateAppEnvResponse, error) {
	arg := admin.NewAdminClientUpdateAppEnvArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_UPDATE_APP_ENV")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientUpdateAppEnvResult)
	return ret.GetSuccess(), nil
}

// UpdateAppEnv is auto-generated
func (m *MetaManager) UpdateAppEnv(ctx context.Context, req *admin.ConfigurationUpdateAppEnvRequest) (*admin.ConfigurationUpdateAppEnvResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.updateAppEnv(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.ConfigurationUpdateAppEnvResponse), fmt.Errorf("UpdateAppEnv failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.ConfigurationUpdateAppEnvResponse), nil
	}
	return nil, err
}

func (ms *metaSession) listNodes(ctx context.Context, req *admin.ConfigurationListNodesRequest) (*admin.ConfigurationListNodesResponse, error) {
	arg := admin.NewAdminClientListNodesArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_LIST_NODES")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientListNodesResult)
	return ret.GetSuccess(), nil
}

// ListNodes is auto-generated
func (m *MetaManager) ListNodes(ctx context.Context, req *admin.ConfigurationListNodesRequest) (*admin.ConfigurationListNodesResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.listNodes(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.ConfigurationListNodesResponse), fmt.Errorf("ListNodes failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.ConfigurationListNodesResponse), nil
	}
	return nil, err
}

func (ms *metaSession) queryClusterInfo(ctx context.Context, req *admin.ConfigurationClusterInfoRequest) (*admin.ConfigurationClusterInfoResponse, error) {
	arg := admin.NewAdminClientQueryClusterInfoArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_CLUSTER_INFO")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientQueryClusterInfoResult)
	return ret.GetSuccess(), nil
}

// QueryClusterInfo is auto-generated
func (m *MetaManager) QueryClusterInfo(ctx context.Context, req *admin.ConfigurationClusterInfoRequest) (*admin.ConfigurationClusterInfoResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.queryClusterInfo(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.ConfigurationClusterInfoResponse), fmt.Errorf("QueryClusterInfo failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.ConfigurationClusterInfoResponse), nil
	}
	return nil, err
}

func (ms *metaSession) metaControl(ctx context.Context, req *admin.ConfigurationMetaControlRequest) (*admin.ConfigurationMetaControlResponse, error) {
	arg := admin.NewAdminClientMetaControlArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_CONTROL_META")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientMetaControlResult)
	return ret.GetSuccess(), nil
}

// MetaControl is auto-generated
func (m *MetaManager) MetaControl(ctx context.Context, req *admin.ConfigurationMetaControlRequest) (*admin.ConfigurationMetaControlResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.metaControl(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.ConfigurationMetaControlResponse), fmt.Errorf("MetaControl failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.ConfigurationMetaControlResponse), nil
	}
	return nil, err
}

func (ms *metaSession) queryBackupPolicy(ctx context.Context, req *admin.ConfigurationQueryBackupPolicyRequest) (*admin.ConfigurationQueryBackupPolicyResponse, error) {
	arg := admin.NewAdminClientQueryBackupPolicyArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_QUERY_BACKUP_POLICY")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientQueryBackupPolicyResult)
	return ret.GetSuccess(), nil
}

// QueryBackupPolicy is auto-generated
func (m *MetaManager) QueryBackupPolicy(ctx context.Context, req *admin.ConfigurationQueryBackupPolicyRequest) (*admin.ConfigurationQueryBackupPolicyResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.queryBackupPolicy(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.ConfigurationQueryBackupPolicyResponse), fmt.Errorf("QueryBackupPolicy failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.ConfigurationQueryBackupPolicyResponse), nil
	}
	return nil, err
}

func (ms *metaSession) balance(ctx context.Context, req *admin.ConfigurationBalancerRequest) (*admin.ConfigurationBalancerResponse, error) {
	arg := admin.NewAdminClientBalanceArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_PROPOSE_BALANCER")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientBalanceResult)
	return ret.GetSuccess(), nil
}

// Balance is auto-generated
func (m *MetaManager) Balance(ctx context.Context, req *admin.ConfigurationBalancerRequest) (*admin.ConfigurationBalancerResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.balance(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.ConfigurationBalancerResponse), fmt.Errorf("Balance failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.ConfigurationBalancerResponse), nil
	}
	return nil, err
}

func (ms *metaSession) startBackupApp(ctx context.Context, req *admin.StartBackupAppRequest) (*admin.StartBackupAppResponse, error) {
	arg := admin.NewAdminClientStartBackupAppArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_START_BACKUP_APP")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientStartBackupAppResult)
	return ret.GetSuccess(), nil
}

// StartBackupApp is auto-generated
func (m *MetaManager) StartBackupApp(ctx context.Context, req *admin.StartBackupAppRequest) (*admin.StartBackupAppResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.startBackupApp(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.StartBackupAppResponse), fmt.Errorf("StartBackupApp failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.StartBackupAppResponse), nil
	}
	return nil, err
}

func (ms *metaSession) queryBackupStatus(ctx context.Context, req *admin.QueryBackupStatusRequest) (*admin.QueryBackupStatusResponse, error) {
	arg := admin.NewAdminClientQueryBackupStatusArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_QUERY_BACKUP_STATUS")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientQueryBackupStatusResult)
	return ret.GetSuccess(), nil
}

// QueryBackupStatus is auto-generated
func (m *MetaManager) QueryBackupStatus(ctx context.Context, req *admin.QueryBackupStatusRequest) (*admin.QueryBackupStatusResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.queryBackupStatus(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.QueryBackupStatusResponse), fmt.Errorf("QueryBackupStatus failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.QueryBackupStatusResponse), nil
	}
	return nil, err
}

func (ms *metaSession) restoreApp(ctx context.Context, req *admin.ConfigurationRestoreRequest) (*admin.ConfigurationCreateAppResponse, error) {
	arg := admin.NewAdminClientRestoreAppArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_START_RESTORE")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientRestoreAppResult)
	return ret.GetSuccess(), nil
}

// RestoreApp is auto-generated
func (m *MetaManager) RestoreApp(ctx context.Context, req *admin.ConfigurationRestoreRequest) (*admin.ConfigurationCreateAppResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.restoreApp(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.ConfigurationCreateAppResponse), fmt.Errorf("RestoreApp failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.ConfigurationCreateAppResponse), nil
	}
	return nil, err
}

func (ms *metaSession) startPartitionSplit(ctx context.Context, req *admin.StartPartitionSplitRequest) (*admin.StartPartitionSplitResponse, error) {
	arg := admin.NewAdminClientStartPartitionSplitArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_START_PARTITION_SPLIT")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientStartPartitionSplitResult)
	return ret.GetSuccess(), nil
}

// StartPartitionSplit is auto-generated
func (m *MetaManager) StartPartitionSplit(ctx context.Context, req *admin.StartPartitionSplitRequest) (*admin.StartPartitionSplitResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.startPartitionSplit(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.StartPartitionSplitResponse), fmt.Errorf("StartPartitionSplit failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.StartPartitionSplitResponse), nil
	}
	return nil, err
}

func (ms *metaSession) querySplitStatus(ctx context.Context, req *admin.QuerySplitRequest) (*admin.QuerySplitResponse, error) {
	arg := admin.NewAdminClientQuerySplitStatusArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_QUERY_PARTITION_SPLIT")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientQuerySplitStatusResult)
	return ret.GetSuccess(), nil
}

// QuerySplitStatus is auto-generated
func (m *MetaManager) QuerySplitStatus(ctx context.Context, req *admin.QuerySplitRequest) (*admin.QuerySplitResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.querySplitStatus(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.QuerySplitResponse), fmt.Errorf("QuerySplitStatus failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.QuerySplitResponse), nil
	}
	return nil, err
}

func (ms *metaSession) controlPartitionSplit(ctx context.Context, req *admin.ControlSplitRequest) (*admin.ControlSplitResponse, error) {
	arg := admin.NewAdminClientControlPartitionSplitArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_CONTROL_PARTITION_SPLIT")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientControlPartitionSplitResult)
	return ret.GetSuccess(), nil
}

// ControlPartitionSplit is auto-generated
func (m *MetaManager) ControlPartitionSplit(ctx context.Context, req *admin.ControlSplitRequest) (*admin.ControlSplitResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.controlPartitionSplit(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.ControlSplitResponse), fmt.Errorf("ControlPartitionSplit failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.ControlSplitResponse), nil
	}
	return nil, err
}

func (ms *metaSession) startBulkLoad(ctx context.Context, req *admin.StartBulkLoadRequest) (*admin.StartBulkLoadResponse, error) {
	arg := admin.NewAdminClientStartBulkLoadArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_START_BULK_LOAD")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientStartBulkLoadResult)
	return ret.GetSuccess(), nil
}

// StartBulkLoad is auto-generated
func (m *MetaManager) StartBulkLoad(ctx context.Context, req *admin.StartBulkLoadRequest) (*admin.StartBulkLoadResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.startBulkLoad(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.StartBulkLoadResponse), fmt.Errorf("StartBulkLoad failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.StartBulkLoadResponse), nil
	}
	return nil, err
}

func (ms *metaSession) queryBulkLoadStatus(ctx context.Context, req *admin.QueryBulkLoadRequest) (*admin.QueryBulkLoadResponse, error) {
	arg := admin.NewAdminClientQueryBulkLoadStatusArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_QUERY_BULK_LOAD_STATUS")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientQueryBulkLoadStatusResult)
	return ret.GetSuccess(), nil
}

// QueryBulkLoadStatus is auto-generated
func (m *MetaManager) QueryBulkLoadStatus(ctx context.Context, req *admin.QueryBulkLoadRequest) (*admin.QueryBulkLoadResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.queryBulkLoadStatus(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.QueryBulkLoadResponse), fmt.Errorf("QueryBulkLoadStatus failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.QueryBulkLoadResponse), nil
	}
	return nil, err
}

func (ms *metaSession) controlBulkLoad(ctx context.Context, req *admin.ControlBulkLoadRequest) (*admin.ControlBulkLoadResponse, error) {
	arg := admin.NewAdminClientControlBulkLoadArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_CONTROL_BULK_LOAD")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientControlBulkLoadResult)
	return ret.GetSuccess(), nil
}

// ControlBulkLoad is auto-generated
func (m *MetaManager) ControlBulkLoad(ctx context.Context, req *admin.ControlBulkLoadRequest) (*admin.ControlBulkLoadResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.controlBulkLoad(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.ControlBulkLoadResponse), fmt.Errorf("ControlBulkLoad failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.ControlBulkLoadResponse), nil
	}
	return nil, err
}

func (ms *metaSession) clearBulkLoad(ctx context.Context, req *admin.ClearBulkLoadStateRequest) (*admin.ClearBulkLoadStateResponse, error) {
	arg := admin.NewAdminClientClearBulkLoadArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_CLEAR_BULK_LOAD")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientClearBulkLoadResult)
	return ret.GetSuccess(), nil
}

// ClearBulkLoad is auto-generated
func (m *MetaManager) ClearBulkLoad(ctx context.Context, req *admin.ClearBulkLoadStateRequest) (*admin.ClearBulkLoadStateResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.clearBulkLoad(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.ClearBulkLoadStateResponse), fmt.Errorf("ClearBulkLoad failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.ClearBulkLoadStateResponse), nil
	}
	return nil, err
}

func (ms *metaSession) startManualCompact(ctx context.Context, req *admin.StartAppManualCompactRequest) (*admin.StartAppManualCompactResponse, error) {
	arg := admin.NewAdminClientStartManualCompactArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_START_MANUAL_COMPACT")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientStartManualCompactResult)
	return ret.GetSuccess(), nil
}

// StartManualCompact is auto-generated
func (m *MetaManager) StartManualCompact(ctx context.Context, req *admin.StartAppManualCompactRequest) (*admin.StartAppManualCompactResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.startManualCompact(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.StartAppManualCompactResponse), fmt.Errorf("StartManualCompact failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.StartAppManualCompactResponse), nil
	}
	return nil, err
}

func (ms *metaSession) queryManualCompact(ctx context.Context, req *admin.QueryAppManualCompactRequest) (*admin.QueryAppManualCompactResponse, error) {
	arg := admin.NewAdminClientQueryManualCompactArgs()
	arg.Req = req
	result, err := ms.call(ctx, arg, "RPC_CM_QUERY_MANUAL_COMPACT_STATUS")
	if err != nil {
		return nil, fmt.Errorf("RPC to session %s failed: %s", ms, err)
	}
	ret, _ := result.(*admin.AdminClientQueryManualCompactResult)
	return ret.GetSuccess(), nil
}

// QueryManualCompact is auto-generated
func (m *MetaManager) QueryManualCompact(ctx context.Context, req *admin.QueryAppManualCompactRequest) (*admin.QueryAppManualCompactResponse, error) {
	resp, err := m.call(ctx, func(rpcCtx context.Context, ms *metaSession) (metaResponse, error) {
		return ms.queryManualCompact(rpcCtx, req)
	})
	if err == nil {
		if resp.GetErr().Errno != base.ERR_OK.String() {
			return resp.(*admin.QueryAppManualCompactResponse), fmt.Errorf("QueryManualCompact failed: %s", resp.GetErr().String())
		}
		return resp.(*admin.QueryAppManualCompactResponse), nil
	}
	return nil, err
}
