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

func GetAllAgentsBasicInfo() []entity.Agent {
	var agentInfoList []entity.Agent
	s.DB.Find(&agentInfoList)
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
	var err error
	if filed == nil {
		err = s.DB.Model(agent).Updates(*agent).Error
		return err
	}
	err = s.DB.Model(agent).Select(filed).Updates(*agent).Error
	return err
}

func CreateOrUpdateAgentBasicInfo(conflictColumnNames []string, agent ...*entity.Agent) error {
	return createOrUpdateEntities(conflictColumnNames, nil, agent...)
}
