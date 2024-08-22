package repository

import (
	"config-server2/internal/entity"
)

func CreateOrUpdateAgentPipelineConfigs(conflictColumnNames []string, assignmentColumns []string, configs ...*entity.AgentPipelineConfig) error {
	return createOrUpdateEntities(conflictColumnNames, assignmentColumns, configs...)
}

func CreateOrUpdateAgentInstanceConfigs(conflictColumnNames []string, assignmentColumns []string, configs ...*entity.AgentInstanceConfig) error {
	return createOrUpdateEntities(conflictColumnNames, assignmentColumns, configs...)
}

func GetPipelineConfigs(agent *entity.Agent) error {
	return s.DB.Preload("PipelineConfigs").Find(agent).Error
}

func GetInstanceConfigs(agent *entity.Agent) error {
	return s.DB.Preload("InstanceConfigs").Find(agent).Error
}
