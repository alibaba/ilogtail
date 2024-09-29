package repository

import (
	"config-server/internal/common"
	"config-server/internal/entity"
)

func CreatePipelineConfig(config *entity.PipelineConfig) error {
	row := s.Db.Create(config).RowsAffected
	if row != 1 {
		return common.ServerErrorWithMsg("create pipelineConfig(%s) error", config.Name)
	}
	return nil
}

func UpdatePipelineConfig(config *entity.PipelineConfig) error {
	err := s.Db.Save(config).Error
	return err
}

func DeletePipelineConfig(configName string) error {
	row := s.Db.Where("name=?", configName).Delete(&entity.PipelineConfig{}).RowsAffected
	if row != 1 {
		return common.ServerErrorWithMsg("delete pipelineConfig(name=%s) error", configName)
	}
	return nil
}

func GetPipelineConfig(configName string) (*entity.PipelineConfig, error) {
	config := &entity.PipelineConfig{}
	row := s.Db.Where("name=?", configName).Find(config).RowsAffected
	if row != 1 {
		return nil, common.ServerErrorWithMsg("delete pipelineConfig(name=%s) error", configName)
	}
	return config, nil
}

func ListPipelineConfigs() ([]*entity.PipelineConfig, error) {
	pipelineConfigs := make([]*entity.PipelineConfig, 0)
	err := s.Db.Find(&pipelineConfigs).Error
	return pipelineConfigs, err
}

func CreatePipelineConfigForAgentGroup(groupName string, configName string) error {
	removeConfig := entity.PipelineConfig{
		Name: configName,
	}
	agentGroup := entity.AgentGroup{
		Name: groupName,
	}
	err := s.Db.Model(&agentGroup).Association("PipelineConfigs").Append(&removeConfig)
	return common.SystemError(err)
}

func CreatePipelineConfigForAgent(instanceId string, configName string) error {
	agentPipelineConfig := entity.AgentPipelineConfig{
		AgentInstanceId:    instanceId,
		PipelineConfigName: configName,
	}
	err := s.Db.FirstOrCreate(&agentPipelineConfig).Error
	return err
}

//func CreatePipelineConfigForAgentInGroup(agentInstanceIds []string, configName string) error {
//	agentPipelineConfigs := make([]entity.AgentPipelineConfig, 0)
//	for _, instanceId := range agentInstanceIds {
//		agentPipelineConfig := entity.AgentPipelineConfig{
//			AgentInstanceId:    instanceId,
//			PipelineConfigName: configName,
//		}
//		agentPipelineConfigs = append(agentPipelineConfigs, agentPipelineConfig)
//	}
//	if len(agentPipelineConfigs) > 0 {
//		err := s.Db.Create(&agentPipelineConfigs).Error
//		return common.SystemError(err)
//	}
//	return nil
//}

func DeletePipelineConfigForAgentGroup(groupName string, configName string) error {
	removeConfig := entity.PipelineConfig{
		Name: configName,
	}
	agentGroup := entity.AgentGroup{
		Name: groupName,
	}
	err := s.Db.Model(&agentGroup).Association("PipelineConfigs").Delete(&removeConfig)
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
		err := s.Db.Delete(&agentPipelineConfigs).Error
		return common.SystemError(err)
	}
	return nil
}

func DeleteAllPipelineConfigAndAgent() {
	s.Db.Exec("TRUNCATE TABLE agent_pipeline_config")
}

func ListAgentPipelineConfig() []*entity.AgentPipelineConfig {
	agentPipelineConfigs := make([]*entity.AgentPipelineConfig, 0)
	s.Db.Find(&agentPipelineConfigs)
	return agentPipelineConfigs
}
