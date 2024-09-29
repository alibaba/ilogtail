package entity

import (
	proto "config-server/internal/protov2"
)

type AgentPipelineConfig struct {
	AgentInstanceId    string `gorm:"primarykey"`
	PipelineConfigName string `gorm:"primarykey"`
	Status             ConfigStatus
	Message            string
}

func (apc AgentPipelineConfig) TableName() string {
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

func (apc AgentPipelineConfig) Equals(obj any) bool {
	if a1, ok := obj.(AgentPipelineConfig); ok {
		return apc.AgentInstanceId == a1.AgentInstanceId && apc.PipelineConfigName == a1.PipelineConfigName
	}
	if a1, ok := obj.(*AgentPipelineConfig); ok {
		return apc.AgentInstanceId == a1.AgentInstanceId && apc.PipelineConfigName == a1.PipelineConfigName
	}
	return false
}

func (apc AgentPipelineConfig) Parse2ProtoAgentConfigStatus() *proto.AgentConfigStatus {
	return &proto.AgentConfigStatus{
		Name:    apc.PipelineConfigName,
		Status:  proto.ConfigStatus(apc.Status),
		Message: apc.Message,
	}
}

type AgentInstanceConfig struct {
	AgentInstanceId    string `gorm:"primarykey"`
	InstanceConfigName string `gorm:"primarykey"`
	Status             ConfigStatus
	Message            string
}

func (aic AgentInstanceConfig) TableName() string {
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

func (aic AgentInstanceConfig) Parse2ProtoAgentConfigStatus() *proto.AgentConfigStatus {
	return &proto.AgentConfigStatus{
		Name:    aic.InstanceConfigName,
		Status:  proto.ConfigStatus(aic.Status),
		Message: aic.Message,
	}
}

func (aic AgentInstanceConfig) Equals(obj any) bool {
	if a1, ok := obj.(AgentInstanceConfig); ok {
		return aic.AgentInstanceId == a1.AgentInstanceId && aic.InstanceConfigName == a1.InstanceConfigName
	}
	if a1, ok := obj.(*AgentInstanceConfig); ok {
		return aic.AgentInstanceId == a1.AgentInstanceId && aic.InstanceConfigName == a1.InstanceConfigName
	}
	return false
}
