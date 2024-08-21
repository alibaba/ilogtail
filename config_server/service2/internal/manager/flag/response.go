package flag

import (
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/config"
	"config-server2/internal/manager/capability"
)

type ResponseAction struct {
	Code int
	Run1 func(*proto.HeartbeatRequest, *proto.HeartbeatResponse) error
	Run2 func(*proto.HeartbeatRequest, *proto.HeartbeatResponse) error
}

const (
	Unspecified               = "unspecified"
	ReportFullState           = "reportFullState"
	FetchPipelineConfigDetail = "fetchPipelineConfigDetail"
	FetchProcessConfigDetail  = "fetchProcessConfigDetail"
)

var ResponseMap = map[string]*ResponseAction{
	Unspecified: {
		Code: 0,
		Run1: ResponseUnspecifiedRun,
	},
	ReportFullState: {
		Code: 1,
		Run1: ResponseReportFullStateRun,
	},
	FetchPipelineConfigDetail: {
		Code: 2,
		Run1: FetchPipelineConfigDetailRun,
		Run2: FetchPipelineConfigDetailIncludeDetailRun,
	},
	FetchProcessConfigDetail: {
		Code: 4,
		Run1: FetchProcessConfigDetailRun,
		Run2: FetchProcessConfigDetailIncludeDetailRun,
	},
}

func ResponseUnspecifiedRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	return nil
}

func ResponseReportFullStateRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	return nil
}

func FetchPipelineConfigDetailIncludeDetailRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	err := capability.AcceptsPipelineConfigAction.Action(req, res, true)
	return err
}

// FetchPipelineConfigDetailRun 要求agent做的事情，比如配置不发送pipelineConfig的Detail,
// 这里就要求它主动请求FetchPipelineConfig接口(只需改变res.flags并且configUpdate不包含detail）
func FetchPipelineConfigDetailRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	err := capability.AcceptsPipelineConfigAction.Action(req, res, false)
	return err
}

func FetchProcessConfigDetailRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	err := capability.AcceptsProcessConfigAction.Action(req, res, false)
	return err
}

func FetchProcessConfigDetailIncludeDetailRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	err := capability.AcceptsProcessConfigAction.Action(req, res, true)
	return err
}

func HandleResponseFlags(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	for key, value := range config.ServerConfigInstance.ResponseFlags {
		action, ok := ResponseMap[key]
		if !ok {
			panic("not the correct responseFlags config...")
		}
		if value {
			res.Flags = res.Flags | uint64(action.Code)
			if action.Run1 == nil {
				continue
			}
			err := action.Run1(req, res)
			if err != nil {
				return err
			}
		} else {
			if action.Run2 == nil {
				continue
			}
			err := action.Run2(req, res)
			if err != nil {
				return err
			}
		}
	}
	return nil
}
