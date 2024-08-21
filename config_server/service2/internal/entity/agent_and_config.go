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

func ProtoConfigInfoParse2AgentProcessConfig(instanceId string, configs []*proto.ConfigInfo) []*AgentProcessConfig {
	agentProcessConfigs := make([]*AgentProcessConfig, 0)
	for _, c := range configs {
		agentProcessConfigs = append(agentProcessConfigs, &AgentProcessConfig{
			AgentInstanceId:   instanceId,
			ProcessConfigName: c.Name,
			Status:            ConfigStatus(c.Status),
			Message:           c.Message,
		})
	}
	return agentProcessConfigs
}

type AgentProcessConfig struct {
	AgentInstanceId   string `gorm:"primarykey"`
	ProcessConfigName string `gorm:"primarykey"`
	Status            ConfigStatus
	Message           string
}

func (AgentProcessConfig) TableName() string {
	return agentProcessConfigTable
}
