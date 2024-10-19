package state

import (
	"config-server/common"
	proto "config-server/protov2"
)

type RequestAction struct {
	Base
	Run func(*proto.HeartbeatRequest, *proto.HeartbeatResponse) error
}

var (
	RequestUnspecified     = Base{0, "unspecified"}
	RequestReportFullState = Base{1, "reportFullState"}
)

var requestActionList = []*RequestAction{
	{
		Base: RequestUnspecified,
		Run:  RequestUnspecifiedRun,
	},
	{
		Base: RequestReportFullState,
		Run:  RequestReportFullStateRun,
	},
}

// RequestUnspecifiedRun agent上报简单信息
func RequestUnspecifiedRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	return nil
}

// RequestReportFullStateRun agent上传全量信息
func RequestReportFullStateRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	for _, action := range ServerActionList {
		err := action.Action(req, res)
		if err != nil {
			return common.SystemError(err)
		}
	}
	return nil
}

func HandleRequestFlags(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	err := HandleServerCapabilities(res)
	if err != nil {
		return common.SystemError(err)
	}

	for _, action := range requestActionList {
		code := action.Code
		if int(req.Flags)&code == code {
			err := action.Run(req, res)
			if err != nil {
				return common.SystemError(err)
			}
		}
	}
	return nil
}
