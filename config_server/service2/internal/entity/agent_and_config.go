package entity

import proto "config-server2/internal/common/protov2"

type AgentPipelineConfig struct {
	AgentInstanceId    string `gorm:"primarykey"`
	PipelineConfigName string `gorm:"primarykey"`
	Status             ConfigStatus
	Message            string
}

func (AgentPipelineConfig) TableName() string {
	return agentPipelineConfigTable
}

func ProtoConfigInfoParse2AgentPipelineConfig(instanceId string, configs []*proto.ConfigInfo) []*AgentPipelineConfig {
	agentPipelineConfigs := make([]*AgentPipelineConfig, 0)

	for _, c := range configs {
		agentPipelineConfigs = append(agentPipelineConfigs, &AgentPipelineConfig{
			AgentInstanceId:    instanceId,
			PipelineConfigName: c.Name,
			Status:             ConfigStatus(c.Status),
			Message:            c.Message,
		})
	}
	return agentPipelineConfigs
}

func ProtoConfigInfoParse2AgentInstanceConfig(instanceId string, configs []*proto.ConfigInfo) []*AgentInstanceConfig {
	agentInstanceConfigs := make([]*AgentInstanceConfig, 0)
	for _, c := range configs {
		agentInstanceConfigs = append(agentInstanceConfigs, &AgentInstanceConfig{
			AgentInstanceId:    instanceId,
			InstanceConfigName: c.Name,
			Status:             ConfigStatus(c.Status),
			Message:            c.Message,
		})
	}
	return agentInstanceConfigs
}

type AgentInstanceConfig struct {
	AgentInstanceId    string `gorm:"primarykey"`
	InstanceConfigName string `gorm:"primarykey"`
	Status             ConfigStatus
	Message            string
}

func (AgentInstanceConfig) TableName() string {
	return agentInstanceConfigTable
}
