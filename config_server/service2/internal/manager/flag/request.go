package flag

import (
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/manager/capability"
)

const (
	RequestUnspecified     = "unspecified"
	RequestReportFullState = "reportFullState"
)

type RequestAction struct {
	Value string
	Run   func(*proto.HeartbeatRequest, *proto.HeartbeatResponse) error
}

var RequestMap = map[int]*RequestAction{
	0: {
		Value: RequestUnspecified,
		Run:   RequestUnspecifiedRun,
	},
	1: {
		Value: RequestReportFullState,
		Run:   RequestReportFullStateRun,
	},
}

// RequestUnspecifiedRun agent上报简单信息
func RequestUnspecifiedRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	for _, action := range capability.ServerActionList {
		action.UpdateCapabilities(res)
	}
	return nil
}

// RequestReportFullStateRun agent上传全量信息
func RequestReportFullStateRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	for _, action := range capability.ServerActionList {
		action.UpdateCapabilities(res)
		err := action.Action(req, res)
		if err != nil {
			return err
		}
	}
	return nil
}

func HandleRequestFlags(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	for key, value := range RequestMap {
		if int(req.Flags)&key == key {
			err := value.Run(req, res)
			if err != nil {
				return err
			}
		}
	}
	return nil
}
