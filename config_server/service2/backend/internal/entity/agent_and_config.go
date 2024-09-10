package entity

import (
	proto "config-server2/internal/protov2"
)

type AgentPipelineConfig struct {
	AgentInstanceId    string `gorm:"primarykey"`
	PipelineConfigName string `gorm:"primarykey"`
	Status             ConfigStatus
	Message            string
}

func (AgentPipelineConfig) TableName() string {
	return agentPipelineConfigTable
}

func ParseProtoConfigInfo2AgentPipelineConfig(instanceId string, c *proto.ConfigInfo) *AgentPipelineConfig {
	return &AgentPipelineConfig{
		AgentInstanceId:    instanceId,
		PipelineConfigName: c.Name,
		Status:             ConfigStatus(c.Status),
		Message:            c.Message,
	}
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

func ParseProtoConfigInfo2AgentInstanceConfig(instanceId string, c *proto.ConfigInfo) *AgentInstanceConfig {
	return &AgentInstanceConfig{
		AgentInstanceId:    instanceId,
		InstanceConfigName: c.Name,
		Status:             ConfigStatus(c.Status),
		Message:            c.Message,
	}
}
