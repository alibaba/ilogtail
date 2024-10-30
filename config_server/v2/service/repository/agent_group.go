package repository

import (
	"config-server/common"
	"config-server/entity"
)

func CreateAgentGroup(group *entity.AgentGroup) error {
	row := s.Db.Create(group).RowsAffected
	if row != 1 {
		return common.ServerErrorWithMsg(common.AgentGroupAlreadyExist, "create agentGroup(%s) error", group.Name)
	}
	return nil
}

func UpdateAgentGroup(group *entity.AgentGroup) error {
	err := s.Db.Save(group).Error
	return err
}

func DeleteAgentGroup(name string) error {
	row := s.Db.Where("name=?", name).Delete(&entity.AgentGroup{}).RowsAffected
	if row != 1 {
		return common.ServerErrorWithMsg(common.AgentGroupNotExist, "delete agentGroup(name=%s) error", name)
	}
	return nil
}

func GetAgentGroupDetail(name string, containPipelineConfigs bool, containInstanceConfigs bool) (*entity.AgentGroup, error) {
	agentGroup := &entity.AgentGroup{}
	tx := s.Db.Where("name=?", name)
	if containPipelineConfigs {
		tx.Preload("PipelineConfigs")
	}
	if containInstanceConfigs {
		tx.Preload("InstanceConfigs")
	}
	row := tx.Find(agentGroup).RowsAffected
	if row != 1 {
		return nil, common.ServerErrorWithMsg(common.AgentGroupNotExist, "get agentGroup(name=%s) error", name)
	}
	return agentGroup, nil
}

func GetAllAgentGroupDetail(containAgents bool, containPipelineConfigs bool, containInstanceConfigs bool) ([]*entity.AgentGroup, error) {
	agentGroups := make([]*entity.AgentGroup, 0)
	tx := s.Db.Where("1=1")
	if containAgents {
		tx.Preload("Agents")
	}
	if containPipelineConfigs {
		tx.Preload("PipelineConfigs")
	}
	if containInstanceConfigs {
		tx.Preload("InstanceConfigs")
	}
	err := tx.Find(&agentGroups).Error
	return agentGroups, err
}

func GetAllAgentGroup() ([]*entity.AgentGroup, error) {
	agentGroups := make([]*entity.AgentGroup, 0)
	err := s.Db.Find(&agentGroups).Error
	return agentGroups, err
}

func GetAppliedAgentGroupForPipelineConfigName(configName string) ([]string, error) {
	pipelineConfig := &entity.PipelineConfig{}
	row := s.Db.Preload("AgentGroups").Where("name=?", configName).Find(&pipelineConfig).RowsAffected
	if row != 1 {
		return nil, common.ServerErrorWithMsg(common.AgentGroupNotExist, "can not find name=%s pipelineConfig")
	}
	groupNames := make([]string, 0)
	for _, group := range pipelineConfig.AgentGroups {
		groupNames = append(groupNames, group.Name)
	}
	return groupNames, nil
}
func GetAppliedAgentGroupForInstanceConfigName(configName string) ([]string, error) {
	instanceConfig := &entity.InstanceConfig{}
	row := s.Db.Preload("AgentGroups").Where("name=?", configName).Find(&instanceConfig).RowsAffected
	if row != 1 {
		return nil, common.ServerErrorWithMsg(common.ConfigNotExist, "can not find name=%s pipelineConfig")
	}
	groupNames := make([]string, 0)
	for _, group := range instanceConfig.AgentGroups {
		groupNames = append(groupNames, group.Name)
	}
	return groupNames, nil
}
