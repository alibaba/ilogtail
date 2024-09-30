package manager

import (
	"config-server/common"
	"config-server/entity"
	proto "config-server/protov2"
	"config-server/repository"
)

func CreateOrUpdateAgentInstanceConfigs(agentInstanceConfigs []*entity.AgentInstanceConfig) error {
	conflictColumnNames := []string{"agent_instance_id", "instance_config_name"}
	assignmentColumnNames := []string{"status", "message"}
	err := repository.CreateOrUpdateAgentInstanceConfigs(conflictColumnNames, assignmentColumnNames, agentInstanceConfigs...)
	return common.SystemError(err)
}

func CreateOrUpdateAgentPipelineConfigs(agentPipelineConfigs []*entity.AgentPipelineConfig) error {
	conflictColumnNames := []string{"agent_instance_id", "pipeline_config_name"}
	assignmentColumnNames := []string{"status", "message"}
	err := repository.CreateOrUpdateAgentPipelineConfigs(conflictColumnNames, assignmentColumnNames, agentPipelineConfigs...)
	return common.SystemError(err)
}

func GetPipelineConfigs(instanceId string, isContainDetail bool) ([]*proto.ConfigDetail, error) {
	var err error
	agent := &entity.Agent{InstanceId: instanceId}
	err = repository.GetPipelineConfigsByAgent(agent)
	if err != nil {
		return nil, err
	}

	pipelineConfigUpdates := make([]*proto.ConfigDetail, 0)
	for _, config := range agent.PipelineConfigs {
		detail := config.Parse2ProtoPipelineConfigDetail(isContainDetail)
		pipelineConfigUpdates = append(pipelineConfigUpdates, detail)
	}
	return pipelineConfigUpdates, nil
}

func GetInstanceConfigs(instanceId string, isContainDetail bool) ([]*proto.ConfigDetail, error) {
	var err error
	agent := &entity.Agent{InstanceId: instanceId}
	err = repository.GetInstanceConfigsByAgent(agent)
	if err != nil {
		return nil, err
	}

	instanceConfigUpdates := make([]*proto.ConfigDetail, 0)
	for _, config := range agent.InstanceConfigs {
		detail := config.Parse2ProtoInstanceConfigDetail(isContainDetail)
		instanceConfigUpdates = append(instanceConfigUpdates, detail)
	}
	return instanceConfigUpdates, err
}
