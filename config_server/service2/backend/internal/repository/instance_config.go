package repository

import (
	"config-server2/internal/common"
	"config-server2/internal/entity"
)

func CreateInstanceConfig(config *entity.InstanceConfig) error {
	row := s.DB.Create(config).RowsAffected
	if row != 1 {
		return common.ServerErrorWithMsg("create InstanceConfig(%s) error", config.Name)
	}
	return nil
}

func UpdateInstanceConfig(config *entity.InstanceConfig) error {
	row := s.DB.Model(&config).Updates(config).RowsAffected
	if row != 1 {
		return common.ServerErrorWithMsg("update InstanceConfig(%s) error", config.Name)
	}
	return nil
}

func DeleteInstanceConfig(configName string) error {
	row := s.DB.Where("name=?", configName).Delete(&entity.InstanceConfig{}).RowsAffected
	if row != 1 {
		return common.ServerErrorWithMsg("delete InstanceConfig(name=%s) error", configName)
	}
	return nil
}

func GetInstanceConfig(configName string) (*entity.InstanceConfig, error) {
	config := &entity.InstanceConfig{}
	row := s.DB.Where("name=?", configName).Find(config).RowsAffected
	if row != 1 {
		return nil, common.ServerErrorWithMsg("get instanceConfig(name=%s) error", configName)
	}
	return config, nil
}

func ListInstanceConfigs() ([]*entity.InstanceConfig, error) {
	instanceConfigs := make([]*entity.InstanceConfig, 0)
	err := s.DB.Find(&instanceConfigs).Error
	return instanceConfigs, err
}

func CreateInstanceConfigForAgentGroup(groupName string, configName string) error {
	removeConfig := entity.InstanceConfig{
		Name: configName,
	}
	agentGroup := entity.AgentGroup{
		Name: groupName,
	}
	err := s.DB.Model(&agentGroup).Association("InstanceConfigs").Append(&removeConfig)
	return common.SystemError(err)
}

func CreateInstanceConfigForAgentInGroup(agentInstanceIds []string, configName string) error {
	agentInstanceConfigs := make([]entity.AgentInstanceConfig, 0)
	for _, instanceId := range agentInstanceIds {
		agentInstanceConfig := entity.AgentInstanceConfig{
			AgentInstanceId:    instanceId,
			InstanceConfigName: configName,
		}
		agentInstanceConfigs = append(agentInstanceConfigs, agentInstanceConfig)
	}

	if len(agentInstanceConfigs) > 0 {
		err := s.DB.Create(agentInstanceConfigs).Error
		return common.SystemError(err)
	}

	return nil
}

func DeleteInstanceConfigForAgentGroup(groupName string, configName string) error {
	removeConfig := entity.InstanceConfig{
		Name: configName,
	}
	agentGroup := entity.AgentGroup{
		Name: groupName,
	}
	err := s.DB.Model(&agentGroup).Association("InstanceConfigs").Delete(&removeConfig)
	return common.SystemError(err)
}

func DeleteInstanceConfigForAgentInGroup(agentInstanceIds []string, configName string) error {
	agentInstanceConfigs := make([]entity.AgentInstanceConfig, 0)
	for _, instanceId := range agentInstanceIds {
		agentInstanceConfig := entity.AgentInstanceConfig{
			AgentInstanceId:    instanceId,
			InstanceConfigName: configName,
		}
		agentInstanceConfigs = append(agentInstanceConfigs, agentInstanceConfig)
	}
	if len(agentInstanceConfigs) > 0 {
		err := s.DB.Delete(agentInstanceConfigs).Error
		return common.SystemError(err)
	}
	return nil
}
