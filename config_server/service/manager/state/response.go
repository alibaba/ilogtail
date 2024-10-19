package state

import (
	"config-server/common"
	"config-server/config"
	"config-server/manager"
	proto "config-server/protov2"
)

type ResponseAction struct {
	Base
	Run func(*proto.HeartbeatRequest, *proto.HeartbeatResponse) error
}

func (r ResponseAction) UpdateFlags(res *proto.HeartbeatResponse) error {
	responseFlagType := r.Value
	hasResponseFlag := config.ServerConfigInstance.ResponseFlags[responseFlagType]

	if hasResponseFlag {
		res.Flags = res.Flags | uint64(r.Code)
	}
	return nil
}

var (
	Unspecified               = Base{0, "unspecified"}
	ReportFullState           = Base{1, "reportFullState"}
	FetchPipelineConfigDetail = Base{2, "fetchPipelineConfigDetail"}
	FetchInstanceConfigDetail = Base{4, "fetchInstanceConfigDetail"}
)

var (
	UnspecifiedResponseAction               = new(ResponseAction)
	ReportFullStateResponseAction           = new(ResponseAction)
	FetchPipelineConfigDetailResponseAction = new(ResponseAction)
	FetchInstanceConfigDetailResponseAction = new(ResponseAction)
)

func init() {
	UnspecifiedResponseAction.Base = Unspecified
	UnspecifiedResponseAction.Run = ResponseUnspecifiedRun

	ReportFullStateResponseAction.Base = ReportFullState
	ReportFullStateResponseAction.Run = ResponseReportFullStateRun

	FetchPipelineConfigDetailResponseAction.Base = FetchPipelineConfigDetail
	FetchPipelineConfigDetailResponseAction.Run = FetchPipelineConfigDetailRun

	FetchInstanceConfigDetailResponseAction.Base = FetchInstanceConfigDetail
	FetchInstanceConfigDetailResponseAction.Run = FetchInstanceConfigDetailRun
}

var ResponseList = []*ResponseAction{
	{
		Base: Unspecified,
		Run:  ResponseUnspecifiedRun,
	},
	{
		Base: ReportFullState,
		Run:  ResponseReportFullStateRun,
	},
	{
		Base: FetchPipelineConfigDetail,
		Run:  FetchPipelineConfigDetailRun,
	},
	{
		Base: FetchInstanceConfigDetail,
		Run:  FetchInstanceConfigDetailRun,
	},
}

func ResponseUnspecifiedRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	fetchPipelineConfigKey := FetchPipelineConfigDetailResponseAction.Value
	needFetchPipelineConfig := config.ServerConfigInstance.ResponseFlags[fetchPipelineConfigKey]
	if !needFetchPipelineConfig {
		strInstanceId := string(req.InstanceId)

		pipelineConfigUpdates, err := manager.GetPipelineConfigs(strInstanceId, req.PipelineConfigs, true)
		if err != nil {
			return common.SystemError(err)
		}
		res.PipelineConfigUpdates = pipelineConfigUpdates
	}

	fetchInstanceConfigKey := FetchInstanceConfigDetailResponseAction.Value
	needFetchInstanceConfig := config.ServerConfigInstance.ResponseFlags[fetchInstanceConfigKey]
	if !needFetchInstanceConfig {
		strInstanceId := string(req.InstanceId)

		instanceConfigUpdates, err := manager.GetInstanceConfigs(strInstanceId, req.InstanceConfigs, true)
		if err != nil {
			return common.SystemError(err)
		}
		res.InstanceConfigUpdates = instanceConfigUpdates
	}
	return nil
}

func ResponseReportFullStateRun(*proto.HeartbeatRequest, *proto.HeartbeatResponse) error {
	return nil
}

// FetchPipelineConfigDetailRun 要求agent做的事情，比如配置不发送pipelineConfig的Detail,
// 这里就要求它主动请求FetchPipelineConfig接口(只需改变res.flags并且configUpdate不包含detail）
func FetchPipelineConfigDetailRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	key := FetchPipelineConfigDetailResponseAction.Value
	needFetchConfig := config.ServerConfigInstance.ResponseFlags[key]
	if needFetchConfig {
		strInstanceId := string(req.InstanceId)
		pipelineConfigUpdates, err := manager.GetPipelineConfigs(strInstanceId, req.PipelineConfigs, false)
		if err != nil {
			return common.SystemError(err)
		}
		res.PipelineConfigUpdates = pipelineConfigUpdates
	}
	return nil
}

func FetchInstanceConfigDetailRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	key := FetchInstanceConfigDetailResponseAction.Value
	needFetchConfig := config.ServerConfigInstance.ResponseFlags[key]
	if needFetchConfig {
		strInstanceId := string(req.InstanceId)

		instanceConfigUpdates, err := manager.GetInstanceConfigs(strInstanceId, req.InstanceConfigs, false)
		if err != nil {
			return common.SystemError(err)
		}
		res.InstanceConfigUpdates = instanceConfigUpdates
	}
	return nil
}

func HandleResponseFlags(res *proto.HeartbeatResponse) error {
	for _, flag := range ResponseList {
		err := flag.UpdateFlags(res)
		if err != nil {
			return common.SystemError(err)
		}
	}
	return nil
}

//func HandleResponseFlags(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
//	for key, value := range config.ServerConfigInstance.ResponseFlags {
//		action, ok := ResponseMap[key]
//		if !ok {
//			panic("not the correct responseFlags config...")
//		}
//		if value {
//			res.Flags = res.Flags | uint64(action.Code)
//			if action.Run1 == nil {
//				continue
//			}
//			err := action.Run1(req, res)
//			if err != nil {
//				return common.SystemError(err)
//			}
//		} else {
//			if action.Run2 == nil {
//				continue
//			}
//			err := action.Run2(req, res)
//			if err != nil {
//				return common.SystemError(err)
//			}
//		}
//	}
//	return nil
//}
