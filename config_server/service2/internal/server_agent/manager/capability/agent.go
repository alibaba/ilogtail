package capability

import (
	"config-server2/internal/common"
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/server_agent/manager"
	"log"
)

type AgentAction struct {
	Base
	run func(*proto.HeartbeatRequest, *proto.HeartbeatResponse, ...any) error
}

func (a AgentAction) Action(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse, arg ...any) error {
	code := a.Code
	if int(req.Capabilities)&code == code {
		err := a.run(req, res, arg...)
		return err
	}
	return nil
}

var (
	AgentUnSpecifiedAction = &AgentAction{
		Base: AgentUnSpecified,
		run:  UnspecifiedRun,
	}
	AcceptsPipelineConfigAction = &AgentAction{
		Base: AcceptsPipelineConfig,
		run:  AcceptsPipelineConfigRun,
	}
	AcceptsInstanceConfigAction = AgentAction{
		Base: AcceptsInstanceConfig,
		run:  AcceptsInstanceConfigRun,
	}
	AcceptsCustomCommandAction = &AgentAction{
		Base: AcceptsCustomCommand,
		run:  AcceptsCustomCommandRun,
	}
)

//agent有接收某种配置的能力，则响应中设置对应的配置

func UnspecifiedRun(*proto.HeartbeatRequest, *proto.HeartbeatResponse, ...any) error {
	return nil
}

// AcceptsPipelineConfigRun 返回PipelineConfigDetail
func AcceptsPipelineConfigRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse, arg ...any) error {
	if len(arg) == 0 {
		return common.ServerErrorWithMsg("The arg parameter cannot be null")
	}

	if isContainDetail, ok := arg[0].(bool); ok {
		strInstanceId := string(req.InstanceId)
		pipelineConfigUpdates, err := manager.GetPipelineConfigs(strInstanceId, isContainDetail)
		if err != nil {
			return err
		}
		res.PipelineConfigUpdates = pipelineConfigUpdates
		return nil
	}
	return common.ServerErrorWithMsg("Arg is illegal, what is required is a boolean type parameter")
}

// AcceptsInstanceConfigRun 返回InstanceConfigDetail
func AcceptsInstanceConfigRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse, arg ...any) error {
	if len(arg) == 0 {
		return common.ServerErrorWithMsg("The arg parameter cannot be null")
	}

	if isContainDetail, ok := arg[0].(bool); ok {
		strInstanceId := string(req.InstanceId)
		instanceConfigUpdates, err := manager.GetInstanceConfigs(strInstanceId, isContainDetail)
		if err != nil {
			return err
		}
		res.InstanceConfigUpdates = instanceConfigUpdates
		return nil
	}
	return common.ServerErrorWithMsg("Arg is illegal, what is required is a boolean type parameter")
}

func AcceptsCustomCommandRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse, arg ...any) error {
	log.Print("command capability action run ...")
	return nil
}
