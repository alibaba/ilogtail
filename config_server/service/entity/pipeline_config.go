package entity

import (
	proto "config-server/protov2"
	"database/sql/driver"
	"encoding/json"
	"errors"
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

func (p PipelineConfigStatus) Parse2ProtoConfigInfo() *proto.ConfigInfo {
	return &proto.ConfigInfo{
		Name:    p.Name,
		Version: p.Version,
		Status:  proto.ConfigStatus(p.Status),
		Message: p.Message,
	}
}

type PipelineConfigStatusList []*PipelineConfigStatus

func (p PipelineConfigStatusList) Parse2ProtoConfigStatus() []*proto.ConfigInfo {
	protoConfigInfos := make([]*proto.ConfigInfo, 0)
	for _, status := range p {
		protoConfigInfos = append(protoConfigInfos, status.Parse2ProtoConfigInfo())
	}
	return protoConfigInfos
}

func ParsePipelineConfigStatusList2Proto(configInfos []*proto.ConfigInfo) PipelineConfigStatusList {
	res := make(PipelineConfigStatusList, 0)
	for _, configInfo := range configInfos {
		res = append(res, ParseProtoPipelineConfigStatus2PipelineConfigStatus(configInfo))
	}
	return res
}

func (p PipelineConfigStatusList) Value() (driver.Value, error) {
	return json.Marshal(p)
}

func (p *PipelineConfigStatusList) Scan(value any) error {
	bytes, ok := value.([]byte)
	if !ok {
		return errors.New("type assertion to []byte failed")
	}
	return json.Unmarshal(bytes, p)
}
