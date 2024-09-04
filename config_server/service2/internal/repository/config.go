package repository

import (
	"config-server2/internal/common"
	"config-server2/internal/entity"
)

func CreateOrUpdateAgentPipelineConfigs(conflictColumnNames []string, assignmentColumns []string, configs ...*entity.AgentPipelineConfig) error {
	err := createOrUpdateEntities(conflictColumnNames, assignmentColumns, configs...)
	return common.SystemError(err)
}

func CreateOrUpdateAgentInstanceConfigs(conflictColumnNames []string, assignmentColumns []string, configs ...*entity.AgentInstanceConfig) error {
	err := createOrUpdateEntities(conflictColumnNames, assignmentColumns, configs...)
	return common.SystemError(err)
}

func GetPipelineConfigByName(name string) (*entity.PipelineConfig, error) {
	pipelineConfig := entity.PipelineConfig{}
	row := s.DB.Where("name=?", name).Find(&pipelineConfig).RowsAffected
	if row != 1 {
		return nil, common.ServerErrorWithMsg("pipelineName=%s can not be found", name)
	}
	return &pipelineConfig, nil
}

func GetInstanceConfigByName(name string) (*entity.InstanceConfig, error) {
	instanceConfig := entity.InstanceConfig{}
	row := s.DB.Where("name=?", name).Find(&instanceConfig).RowsAffected
	if row != 1 {
		return nil, common.ServerErrorWithMsg("instanceName=%s can not be found", name)
	}
	return &instanceConfig, nil
}

func GetPipelineConfigsByAgent(agent *entity.Agent) error {
	err := s.DB.Preload("PipelineConfigs").Where("instance_id=?", agent.InstanceId).Find(agent).Error
	return common.SystemError(err)
}

func GetInstanceConfigsByAgent(agent *entity.Agent) error {
	err := s.DB.Preload("InstanceConfigs").Where("instance_id=?", agent.InstanceId).Find(agent).Error
	return common.SystemError(err)
}
