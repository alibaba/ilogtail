package entity

import proto "config-server2/internal/common/protov2"

type ConfigStatus int32

type PipelineConfig struct {
	Name    string `gorm:"primarykey"`
	Version int64
	Detail  []byte
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

type ProcessConfig struct {
	Name    string `gorm:"primarykey"`
	Version int64
	Detail  []byte
}

func (c ProcessConfig) Parse2ProtoProcessConfigDetail(isContainDetail bool) *proto.ConfigDetail {
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

func (ProcessConfig) TableName() string {
	return processConfigTable
}
