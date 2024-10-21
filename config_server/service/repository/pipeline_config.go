package repository

import (
	"config-server/common"
	"config-server/entity"
)

func GetPipelineConfigByNames(names ...string) ([]*entity.PipelineConfig, error) {
	pipelineConfigs := make([]*entity.PipelineConfig, 0)
	row := s.Db.Where("name in (?)", names).Find(&pipelineConfigs).RowsAffected
	if row != int64(len(names)) {
		return nil, common.ServerErrorWithMsg("pipelineNames=%s can not be found", names)
	}
	return pipelineConfigs, nil
}

func GetPipelineConfigsByAgent(agent *entity.Agent) error {
	err := s.Db.Preload("Tags.PipelineConfigs").Where("instance_id=?", agent.InstanceId).Find(agent).Error
	return common.SystemError(err)
}

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

func DeleteAllPipelineConfigAndAgent() {
	s.Db.Exec("TRUNCATE TABLE agent_pipeline_config")
}
