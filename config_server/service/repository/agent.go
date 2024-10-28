package repository

import (
	"config-server/common"
	"config-server/entity"
	"config-server/store"
)

var s = store.S

func GetAgentByID(instanceId string, fields ...string) (*entity.Agent, error) {
	var agentInfo = &entity.Agent{}
	var row int64
	if fields == nil || len(fields) == 0 {
		row = s.Db.Where("instance_id=?", instanceId).Find(agentInfo).RowsAffected
	} else {
		row = s.Db.Select(fields).Where("instance_id=?", instanceId).Find(agentInfo).RowsAffected
	}
	if row != 1 {
		return agentInfo, common.ServerErrorWithMsg(common.AgentNotExist, "instanceId=%s not exists", instanceId)
	}
	return agentInfo, nil
}

func GetAllAgents(containPipelineConfigs bool, containInstanceConfigs bool) []entity.Agent {
	var agentInfoList []entity.Agent
	tx := s.Db
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
	tx := s.Db.Where("instance_id=?", instanceId).Delete(&entity.Agent{})
	if tx.RowsAffected != 1 {
		return common.ServerErrorWithMsg(common.AgentNotExist, "Agent failed to delete record %s", instanceId)
	}
	return nil
}

func UpdateAgentById(agent *entity.Agent, filed ...string) error {
	if filed == nil {
		err := s.Db.Model(agent).Updates(*agent).Error
		if err != nil {
			return common.SystemError(err)
		}
	}
	err := s.Db.Model(agent).Select(filed).Updates(*agent).Error
	if err != nil {
		return common.SystemError(err)
	}
	return nil
}

func CreateOrUpdateAgentBasicInfo(conflictColumnNames []string, agent ...*entity.Agent) error {
	return createOrUpdateEntities(conflictColumnNames, nil, agent...)
}

func ListAgentsByGroupName(groupName string) ([]*entity.Agent, error) {
	agentGroup := entity.AgentGroup{}
	err := s.Db.Preload("Agents").Where("name=?", groupName).Find(&agentGroup).Error
	if err != nil {
		return nil, err
	}
	return agentGroup.Agents, nil
}
