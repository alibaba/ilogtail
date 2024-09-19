package entity

import (
	proto "config-server/internal/protov2"
)

const AgentGroupDefaultValue = "default"

type AgentGroup struct {
	Name            string `gorm:"primarykey;"`
	Value           string
	Agents          []*Agent          `gorm:"many2many:agent_and_agent_group;foreignKey:Name;joinForeignKey:AgentGroupName;References:InstanceId;joinReferences:AgentInstanceId;constraint:OnUpdate:CASCADE,OnDelete:CASCADE;"`
	PipelineConfigs []*PipelineConfig `gorm:"many2many:agent_group_pipeline_config;foreignKey:Name;joinForeignKey:AgentGroupName;References:Name;joinReferences:PipelineConfigName;constraint:OnUpdate:CASCADE,OnDelete:CASCADE;"`
	InstanceConfigs []*InstanceConfig `gorm:"many2many:agent_group_instance_config;foreignKey:Name;joinForeignKey:AgentGroupName;References:Name;joinReferences:InstanceConfigName;constraint:OnUpdate:CASCADE,OnDelete:CASCADE;"`
}

func (AgentGroup) TableName() string {
	return agentGroupTable
}

func (a AgentGroup) Parse2ProtoAgentGroupTag() *proto.AgentGroupTag {
	return &proto.AgentGroupTag{
		Name:  a.Name,
		Value: a.Value,
	}
}

func ParseProtoAgentGroupTag2AgentGroup(tag *proto.AgentGroupTag) *AgentGroup {
	return &AgentGroup{
		Name:  tag.Name,
		Value: tag.Value,
	}
}
