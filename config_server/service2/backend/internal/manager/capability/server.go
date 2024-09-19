package capability

import (
	"config-server2/internal/common"
	"config-server2/internal/config"
	"config-server2/internal/entity"
	"config-server2/internal/manager"
	proto "config-server2/internal/protov2"
	"config-server2/internal/repository"
)

type ServerAction struct {
	Base
	run func(*proto.HeartbeatRequest, *proto.HeartbeatResponse) error
}

func (a ServerAction) Action(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	serverCapabilityType := a.Value
	hasServerCapability, ok := config.ServerConfigInstance.Capabilities[serverCapabilityType]
	if !ok {
		return common.ServerErrorWithMsg("error serverCapability type(%s)", serverCapabilityType)
	}

	if hasServerCapability {
		err := a.run(req, res)
		return common.SystemError(err)
	}
	return nil
}

func (a ServerAction) UpdateCapabilities(res *proto.HeartbeatResponse) {
	res.Capabilities = res.Capabilities | uint64(a.Code)
}

var (
	UnspecifiedServerAction = &ServerAction{
		Base: ServerUnspecified,
		run:  UnspecifiedServerCapabilityRun,
	}

	RememberAttributeAction = &ServerAction{
		Base: RememberAttribute,
		run:  RememberAttributeCapabilityRun,
	}
	RememberPipelineConfigStatusAction = &ServerAction{
		Base: RememberPipelineConfigStatus,
		run:  RememberPipelineConfigStatusCapabilityRun,
	}
	RememberInstanceConfigStatusAction = &ServerAction{
		Base: RememberInstanceConfigStatus,
		run:  RememberInstanceConfigStatusCapabilityRun,
	}
	RememberCustomCommandStatusAction = &ServerAction{
		Base: RememberCustomCommandStatus,
		run:  RememberCustomCommandStatusRun,
	}
)

var ServerActionList = []*ServerAction{
	UnspecifiedServerAction,
	RememberAttributeAction,
	RememberPipelineConfigStatusAction,
	RememberInstanceConfigStatusAction,
	RememberCustomCommandStatusAction,
}

func UnspecifiedServerCapabilityRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	return nil
}

func RememberAttributeCapabilityRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	attributes := req.Attributes
	if attributes == nil {
		return nil
	}
	agent := &entity.Agent{}
	agent.Attributes = entity.ParseProtoAgentAttributes2AgentAttributes(attributes)
	err := repository.UpdateAgentById(agent, "attributes")
	return common.SystemError(err)
}

func RememberPipelineConfigStatusCapabilityRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	configs := req.PipelineConfigs
	if configs == nil {
		return nil
	}
	agentPipelineConfigs := make([]*entity.AgentPipelineConfig, 0)
	for _, reqPipelineConfig := range req.PipelineConfigs {
		agentPipelineConfig := entity.ParseProtoConfigInfo2AgentPipelineConfig(string(req.InstanceId), reqPipelineConfig)
		agentPipelineConfigs = append(agentPipelineConfigs, agentPipelineConfig)
	}
	err := manager.CreateOrUpdateAgentPipelineConfigs(agentPipelineConfigs)
	return common.SystemError(err)
}

func RememberInstanceConfigStatusCapabilityRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	configs := req.InstanceConfigs
	if configs == nil {
		return nil
	}
	agentInstanceConfigs := make([]*entity.AgentInstanceConfig, 0)
	for _, reqInstanceConfig := range req.InstanceConfigs {
		agentInstanceConfig := entity.ParseProtoConfigInfo2AgentInstanceConfig(string(req.InstanceId), reqInstanceConfig)
		agentInstanceConfigs = append(agentInstanceConfigs, agentInstanceConfig)
	}
	err := manager.CreateOrUpdateAgentInstanceConfigs(agentInstanceConfigs)
	return common.SystemError(err)
}

func RememberCustomCommandStatusRun(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	return nil
}
