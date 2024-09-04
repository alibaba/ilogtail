package repository

import (
	"config-server2/internal/common"
	"config-server2/internal/entity"
)

func CreatePipelineConfig(config *entity.PipelineConfig) error {
	row := s.DB.Create(config).RowsAffected
	if row != 1 {
		return common.ServerErrorWithMsg("create pipelineConfig(%s) error", config.Name)
	}
	return nil
}

func UpdatePipelineConfig(config *entity.PipelineConfig) error {
	row := s.DB.Model(&config).Updates(config).RowsAffected
	if row != 1 {
		return common.ServerErrorWithMsg("update pipelineConfig(%s) error", config.Name)
	}
	return nil
}

func DeletePipelineConfig(configName string) error {
	row := s.DB.Where("name=?", configName).Delete(&entity.PipelineConfig{}).RowsAffected
	if row != 1 {
		return common.ServerErrorWithMsg("delete pipelineConfig(name=%s) error", configName)
	}
	return nil
}

func GetPipelineConfig(configName string) (*entity.PipelineConfig, error) {
	config := &entity.PipelineConfig{}
	row := s.DB.Where("name=?", configName).Find(config).RowsAffected
	if row != 1 {
		return nil, common.ServerErrorWithMsg("delete pipelineConfig(name=%s) error", configName)
	}
	return config, nil
}

func ListPipelineConfigs() ([]*entity.PipelineConfig, error) {
	pipelineConfigs := make([]*entity.PipelineConfig, 0)
	err := s.DB.Find(&pipelineConfigs).Error
	return pipelineConfigs, err
}

func CreatePipelineConfigForAgentGroup(groupName string, configName string) error {
	removeConfig := entity.PipelineConfig{
		Name: configName,
	}
	agentGroup := entity.AgentGroup{
		Name: groupName,
	}
	err := s.DB.Model(&agentGroup).Association("PipelineConfigs").Append(&removeConfig)
	return common.SystemError(err)
}

func CreatePipelineConfigForAgentInGroup(agentInstanceIds []string, configName string) error {
	agentPipelineConfigs := make([]entity.AgentPipelineConfig, 0)
	for _, instanceId := range agentInstanceIds {
		agentPipelineConfig := entity.AgentPipelineConfig{
			AgentInstanceId:    instanceId,
			PipelineConfigName: configName,
		}
		agentPipelineConfigs = append(agentPipelineConfigs, agentPipelineConfig)
	}
	//if len(agentPipelineConfigs) > 0 {
	//	err := s.DB.Create(&agentPipelineConfigs).Error
	//	return common.SystemError(err)
	//}
	err := s.DB.Create(&agentPipelineConfigs).Error
	return common.SystemError(err)
	//return nil
}

func DeletePipelineConfigForAgentGroup(groupName string, configName string) error {
	removeConfig := entity.PipelineConfig{
		Name: configName,
	}
	agentGroup := entity.AgentGroup{
		Name: groupName,
	}
	err := s.DB.Model(&agentGroup).Association("PipelineConfigs").Delete(&removeConfig)
	return common.SystemError(err)
}

func DeletePipelineConfigForAgentInGroup(agentInstanceIds []string, configName string) error {
	agentPipelineConfigs := make([]entity.AgentPipelineConfig, 0)
	for _, instanceId := range agentInstanceIds {
		agentPipelineConfig := entity.AgentPipelineConfig{
			AgentInstanceId:    instanceId,
			PipelineConfigName: configName,
		}
		agentPipelineConfigs = append(agentPipelineConfigs, agentPipelineConfig)
	}
	if len(agentPipelineConfigs) > 0 {
		err := s.DB.Delete(&agentPipelineConfigs).Error
		return common.SystemError(err)
	}
	return nil
}
