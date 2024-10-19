package entity

import (
	proto "config-server/protov2"
)

type PipelineConfig struct {
	Name        string `gorm:"primarykey"`
	Version     int64
	Detail      []byte
	AgentGroups []*AgentGroup `gorm:"many2many:agent_group_pipeline_config;foreignKey:Name;joinForeignKey:PipelineConfigName;References:Name;joinReferences:AgentGroupName;constraint:OnUpdate:CASCADE,OnDelete:CASCADE;"`
}

func (PipelineConfig) TableName() string {
	return pipelineConfigTable
}

func (c PipelineConfig) Parse2ProtoPipelineConfigDetail(isContainDetail bool) *proto.ConfigDetail {
	if isContainDetail {
		return &proto.ConfigDetail{
			Name:    c.Name,
			Version: c.Version,
			Detail:  c.Detail,
		}
	}
	return &proto.ConfigDetail{
		Name:    c.Name,
		Version: c.Version,
	}
}

func ParseProtoPipelineConfig2PipelineConfig(c *proto.ConfigDetail) *PipelineConfig {
	return &PipelineConfig{
		Name:    c.Name,
		Version: c.Version,
		Detail:  c.Detail,
	}
}

type PipelineConfigStatus struct {
	Name    string
	Version int64
	Status  ConfigStatus
	Message string
}

func ParseProtoPipelineConfigStatus2PipelineConfigStatus(c *proto.ConfigInfo) *PipelineConfigStatus {
	return &PipelineConfigStatus{
		Name:    c.Name,
		Version: c.Version,
		Status:  ConfigStatus(c.Status),
		Message: c.Message,
	}
}
