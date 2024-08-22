package entity

import (
	proto "config-server2/internal/common/protov2"
)

const AgentGroupDefaultValue = "default"

type AgentGroup struct {
	Name            string `gorm:"primarykey;"`
	Value           string
	PipelineConfigs []*PipelineConfig `gorm:"many2many:agent_group_pipeline_config;foreignKey:Name;joinForeignKey:AgentGroupName;References:Name;joinReferences:PipelineConfigName;constraint:OnUpdate:CASCADE,OnDelete:CASCADE;"`
	InstanceConfigs []*InstanceConfig `gorm:"many2many:agent_group_instance_config;foreignKey:Name;joinForeignKey:AgentGroupName;References:Name;joinReferences:InstanceConfigName;constraint:OnUpdate:CASCADE,OnDelete:CASCADE;"`
}

func (AgentGroup) TableName() string {
	return agentGroupTable
}

func ParseProtoAgentGroupTag2AgentGroup(tags []*proto.AgentGroupTag) []*AgentGroup {
	var agentTags = make([]*AgentGroup, 0)
	if tags == nil {
		agentTags = append(agentTags, &AgentGroup{
			Name:  AgentGroupDefaultValue,
			Value: AgentGroupDefaultValue,
		})
		return agentTags
	}

	for _, tag := range tags {
		agentTags = append(agentTags, &AgentGroup{
			Name:  tag.Name,
			Value: tag.Value,
		})
	}
	return agentTags
}
