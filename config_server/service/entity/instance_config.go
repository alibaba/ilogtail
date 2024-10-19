package entity

import (
	proto "config-server/protov2"
	"database/sql/driver"
	"encoding/json"
	"errors"
)

type InstanceConfig struct {
	Name        string `gorm:"primarykey"`
	Version     int64
	Detail      []byte
	AgentGroups []*AgentGroup `gorm:"many2many:agent_group_instance_config;foreignKey:Name;joinForeignKey:InstanceConfigName;References:Name;joinReferences:AgentGroupName;constraint:OnUpdate:CASCADE,OnDelete:CASCADE;"`
}

func (InstanceConfig) TableName() string {
	return instanceConfigTable
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

type InstanceConfigStatus struct {
	Name    string
	Version int64
	Status  ConfigStatus
	Message string
}

func ParseProtoInstanceConfigStatus2InstanceConfigStatus(c *proto.ConfigInfo) *InstanceConfigStatus {
	return &InstanceConfigStatus{
		Name:    c.Name,
		Version: c.Version,
		Status:  ConfigStatus(c.Status),
		Message: c.Message,
	}
}

func (i InstanceConfigStatus) Parse2ProtoConfigInfo() *proto.ConfigInfo {
	return &proto.ConfigInfo{
		Name:    i.Name,
		Version: i.Version,
		Status:  proto.ConfigStatus(i.Status),
		Message: i.Message,
	}
}

type InstanceConfigStatusList []*InstanceConfigStatus

func (i InstanceConfigStatusList) ConfigStatus2ProtoConfigStatus() []*proto.ConfigInfo {
	protoConfigInfos := make([]*proto.ConfigInfo, 0)
	for _, status := range i {
		protoConfigInfos = append(protoConfigInfos, status.Parse2ProtoConfigInfo())
	}
	return protoConfigInfos
}

func (i InstanceConfigStatusList) Value() (driver.Value, error) {
	return json.Marshal(i)
}

func (i InstanceConfigStatusList) Scan(value any) error {
	bytes, ok := value.([]byte)
	if !ok {
		return errors.New("type assertion to []byte failed")
	}
	return json.Unmarshal(bytes, &i)
}
