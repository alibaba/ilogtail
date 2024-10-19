package state

import (
	"config-server/common"
	"config-server/config"
	"config-server/entity"
	"config-server/manager"
	proto "config-server/protov2"
	"config-server/repository"
)

type ServerAction struct {
	Base
	Run func(*proto.HeartbeatRequest, *proto.HeartbeatResponse) error
}

func (a ServerAction) Action(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	serverCapabilityType := a.Value
	hasServerCapability, ok := config.ServerConfigInstance.Capabilities[serverCapabilityType]
	if !ok {
		return common.ServerErrorWithMsg("error serverCapability type(%s)", serverCapabilityType)
	}

	if hasServerCapability {
		err := a.Run(req, res)
		return common.SystemError(err)
	}
	return nil
}

func (a ServerAction) UpdateCapabilities(res *proto.HeartbeatResponse) error {
	serverCapabilityType := a.Value
	hasServerCapability := config.ServerConfigInstance.Capabilities[serverCapabilityType]

	if hasServerCapability {
		res.Capabilities = res.Capabilities | uint64(a.Code)
	}
	return nil
}

var (
	ServerUnspecified            = Base{Code: 0, Value: "unspecified"}
	RememberAttribute            = Base{Code: 1, Value: "rememberAttribute"}
	RememberPipelineConfigStatus = Base{Code: 2, Value: "rememberPipelineConfigStatus"}
	RememberInstanceConfigStatus = Base{Code: 4, Value: "rememberInstanceConfigStatus"}
	RememberCustomCommandStatus  = Base{Code: 8, Value: "rememberCustomCommandStatus"}
)

var ServerActionList = []*ServerAction{
	{
		Base: ServerUnspecified,
		Run:  UnspecifiedServerCapabilityRun,
	},
	{
		Base: RememberAttribute,
		Run:  RememberAttributeCapabilityRun,
	},
	{
		Base: RememberPipelineConfigStatus,
		Run:  RememberPipelineConfigStatusCapabilityRun,
	},
	{
		Base: RememberInstanceConfigStatus,
		Run:  RememberInstanceConfigStatusCapabilityRun,
	},
	{
		Base: RememberCustomCommandStatus,
		Run:  RememberCustomCommandStatusRun,
	},
}

func UnspecifiedServerCapabilityRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	return nil
}

func RememberAttributeCapabilityRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	attributes := req.Attributes
	if attributes == nil {
		return nil
	}
	agent := &entity.Agent{
		InstanceId: string(req.InstanceId),
	}
	agent.Attributes = entity.ParseProtoAgentAttributes2AgentAttributes(attributes)
	err := repository.UpdateAgentById(agent, "attributes")
	return common.SystemError(err)
}

func RememberPipelineConfigStatusCapabilityRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	configs := req.PipelineConfigs
	if configs == nil {
		return nil
	}

	err := manager.SavePipelineConfig(configs, string(req.InstanceId))
	return common.SystemError(err)
}

func RememberInstanceConfigStatusCapabilityRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	configs := req.InstanceConfigs
	if configs == nil {
		return nil
	}

	err := manager.SaveInstanceConfig(configs, string(req.InstanceId))
	return common.SystemError(err)
}

func RememberCustomCommandStatusRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	return nil
}

func HandleServerCapabilities(res *proto.HeartbeatResponse) error {
	for _, action := range ServerActionList {
		err := action.UpdateCapabilities(res)
		if err != nil {
			return common.SystemError(err)
		}
	}
	return nil
}
