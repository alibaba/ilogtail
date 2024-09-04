package entity

import proto "config-server2/internal/common/protov2"

type ConfigStatus int32

type PipelineConfig struct {
	Name        string `gorm:"primarykey"`
	Version     int64
	Detail      []byte
	Agents      []*Agent      `gorm:"many2many:agent_pipeline_config;foreignKey:Name;joinForeignKey:PipelineConfigName;References:InstanceId;joinReferences:AgentInstanceId;constraint:OnUpdate:CASCADE,OnDelete:CASCADE;"`
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

type InstanceConfig struct {
	Name        string `gorm:"primarykey"`
	Version     int64
	Detail      []byte
	Agents      []*Agent      `gorm:"many2many:agent_instance_config;foreignKey:Name;joinForeignKey:InstanceConfigName;References:InstanceId;joinReferences:AgentInstanceId;constraint:OnUpdate:CASCADE,OnDelete:CASCADE;"`
	AgentGroups []*AgentGroup `gorm:"many2many:agent_group_instance_config;foreignKey:Name;joinForeignKey:InstanceConfigName;References:Name;joinReferences:AgentGroupName;constraint:OnUpdate:CASCADE,OnDelete:CASCADE;"`
}

func (c InstanceConfig) Parse2ProtoInstanceConfigDetail(isContainDetail bool) *proto.ConfigDetail {
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

func ParseProtoInstanceConfig2InstanceConfig(c *proto.ConfigDetail) *InstanceConfig {
	return &InstanceConfig{
		Name:    c.Name,
		Version: c.Version,
		Detail:  c.Detail,
	}
}

func (InstanceConfig) TableName() string {
	return instanceConfigTable
}
