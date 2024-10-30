package state

import (
	"config-server/common"
	proto "config-server/protov2"
	"log"
)

type AgentAction struct {
	Base
	Run func(*proto.HeartbeatRequest, *proto.HeartbeatResponse) error
}

var (
	AgentUnSpecified      = Base{Code: 0, Value: "unspecified"}
	AcceptsPipelineConfig = Base{Code: 1, Value: "acceptsPipelineConfig"}
	AcceptsInstanceConfig = Base{Code: 2, Value: "acceptsInstanceConfig"}
	AcceptsCustomCommand  = Base{Code: 4, Value: "acceptsCustomCommand"}
)

var agentActionList = []*AgentAction{
	{
		Base: AgentUnSpecified,
		Run:  UnspecifiedRun,
	},
	{
		Base: AcceptsPipelineConfig,
		Run:  AcceptsPipelineConfigRun,
	},
	{
		Base: AcceptsInstanceConfig,
		Run:  AcceptsInstanceConfigRun,
	},
	{
		Base: AcceptsCustomCommand,
		Run:  AcceptsCustomCommandRun,
	},
}

//agent有接收某种配置的能力，则响应中设置对应的配置

func UnspecifiedRun(*proto.HeartbeatRequest, *proto.HeartbeatResponse) error {
	return nil
}

// AcceptsPipelineConfigRun 返回PipelineConfigDetail
func AcceptsPipelineConfigRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	// 心跳检测中返回config
	err := UnspecifiedResponseAction.Run(req, res)
	if err != nil {
		return common.SystemError(err)
	}

	//不在心跳检测中返回，需要额外请求
	err = FetchPipelineConfigDetailResponseAction.Run(req, res)
	if err != nil {
		return common.SystemError(err)
	}
	return nil
}

// AcceptsInstanceConfigRun 返回InstanceConfigDetail
func AcceptsInstanceConfigRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	// 在心跳检测就返回config
	err := UnspecifiedResponseAction.Run(req, res)
	if err != nil {
		return common.SystemError(err)
	}

	err = FetchInstanceConfigDetailResponseAction.Run(req, res)
	if err != nil {
		return common.SystemError(err)
	}
	return nil
}

func AcceptsCustomCommandRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	log.Print("command capability action Run ...")
	return nil
}

func HandleAgentCapabilities(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	err := HandleResponseFlags(res)
	if err != nil {
		return common.SystemError(err)
	}

	for _, action := range agentActionList {
		code := action.Code
		if int(req.Capabilities)&code == code {
			err := action.Run(req, res)
			if err != nil {
				return common.SystemError(err)
			}
		}
	}
	return nil
}
