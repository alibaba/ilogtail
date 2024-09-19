package repository

import (
	"config-server2/internal/common"
	"config-server2/internal/entity"
	"config-server2/internal/store"
	"gorm.io/gorm"
)

var s = store.S

func GetAgentByiId(instanceId string) *entity.Agent {
	var agentInfo = new(entity.Agent)
	row := s.DB.Where("instance_id=?", instanceId).Find(agentInfo).RowsAffected
	if row == 1 {
		return agentInfo
	}
	return nil
}

func GetAllAgents(containPipelineConfigs bool, containInstanceConfigs bool) []entity.Agent {
	var agentInfoList []entity.Agent
	tx := s.DB
	if containPipelineConfigs {
		tx.Preload("PipelineConfigs")
	}
	if containInstanceConfigs {
		tx.Preload("InstanceConfigs")
	}
	tx.Find(&agentInfoList)
	return agentInfoList
}

func RemoveAgentById(instanceId string) error {
	var tx *gorm.DB

	tx = s.DB.Where("instance_id=?", instanceId).Delete(&entity.Agent{})
	if tx.RowsAffected != 1 {
		return common.ServerErrorWithMsg("Agent failed to delete record %s", instanceId)
	}
	s.DB.Where("agent_instance_id=?", instanceId).Delete(&entity.AgentPipelineConfig{})
	s.DB.Where("agent_instance_id=?", instanceId).Delete(&entity.AgentInstanceConfig{})
	return nil
}

func UpdateAgentById(agent *entity.Agent, filed ...string) error {
	if filed == nil {
		row := s.DB.Model(agent).Updates(*agent).RowsAffected
		if row != 1 {
			return common.ServerErrorWithMsg("update agent error")
		}
	}
	row := s.DB.Model(agent).Select(filed).Updates(*agent).RowsAffected
	if row != 1 {
		return common.ServerErrorWithMsg("update agent filed error")
	}
	return nil
}

func CreateOrUpdateAgentBasicInfo(conflictColumnNames []string, agent ...*entity.Agent) error {
	return createOrUpdateEntities(conflictColumnNames, nil, agent...)
}

func ListAgentsByGroupName(groupName string) ([]*entity.Agent, error) {
	agentGroup := entity.AgentGroup{}
	err := s.DB.Preload("Agents").Where("name=?", groupName).Find(&agentGroup).Error
	if err != nil {
		return nil, err
	}
	return agentGroup.Agents, nil
}

func GetPipelineConfigStatusList(instanceId string) ([]*entity.AgentPipelineConfig, error) {
	configs := make([]*entity.AgentPipelineConfig, 0)
	err := s.DB.Where("agent_instance_id=?", instanceId).Find(&configs).Error
	if err != nil {
		return nil, err
	}
	return configs, err
}

func GetInstanceConfigStatusList(instanceId string) ([]*entity.AgentInstanceConfig, error) {
	configs := make([]*entity.AgentInstanceConfig, 0)
	err := s.DB.Where("agent_instance_id=?", instanceId).Find(&configs).Error
	if err != nil {
		return nil, err
	}
	return configs, err
}
