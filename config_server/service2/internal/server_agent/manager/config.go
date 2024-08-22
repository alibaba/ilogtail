package manager

import (
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/entity"
	"config-server2/internal/server_agent/repository"
)

func CreateOrUpdateAgentInstanceConfigs(agentInstanceConfigs []*entity.AgentInstanceConfig) error {
	conflictColumnNames := []string{"agent_instance_id", "instance_config_name"}
	assignmentColumnNames := []string{"status", "message"}
	err := repository.CreateOrUpdateAgentInstanceConfigs(conflictColumnNames, assignmentColumnNames, agentInstanceConfigs...)
	return err
}

func CreateOrUpdateAgentPipelineConfigs(agentPipelineConfigs []*entity.AgentPipelineConfig) error {
	conflictColumnNames := []string{"agent_instance_id", "pipeline_config_name"}
	assignmentColumnNames := []string{"status", "message"}
	err := repository.CreateOrUpdateAgentPipelineConfigs(conflictColumnNames, assignmentColumnNames, agentPipelineConfigs...)
	return err
}

func GetPipelineConfigs(instanceId string, isContainDetail bool) ([]*proto.ConfigDetail, error) {
	var err error
	agent := &entity.Agent{InstanceId: instanceId}
	err = repository.GetPipelineConfigs(agent)
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
	err = repository.GetInstanceConfigs(agent)
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
