package repository

import (
	"config-server/common"
	"config-server/entity"
)

func GetInstanceConfigByNames(names ...string) ([]*entity.InstanceConfig, error) {
	instanceConfigs := make([]*entity.InstanceConfig, 0)
	row := s.Db.Where("name in (?)", names).Find(&instanceConfigs).RowsAffected
	if row != int64(len(names)) {
		return nil, common.ServerErrorWithMsg("instanceNames=%s can not be found", names)
	}
	return instanceConfigs, nil
}

func GetInstanceConfigsByAgent(agent *entity.Agent) error {
	err := s.Db.Preload("Tags.InstanceConfigs").Where("instance_id=?", agent.InstanceId).Find(agent).Error
	return common.SystemError(err)
}

func CreateInstanceConfig(config *entity.InstanceConfig) error {
	row := s.Db.Create(config).RowsAffected
	if row != 1 {
		return common.ServerErrorWithMsg("create InstanceConfig(%s) error", config.Name)
	}
	return nil
}

func UpdateInstanceConfig(config *entity.InstanceConfig) error {
	err := s.Db.Save(config).Error
	return err
}

func DeleteInstanceConfig(configName string) error {
	row := s.Db.Where("name=?", configName).Delete(&entity.InstanceConfig{}).RowsAffected
	if row != 1 {
		return common.ServerErrorWithMsg("delete InstanceConfig(name=%s) error", configName)
	}
	return nil
}

func GetInstanceConfig(configName string) (*entity.InstanceConfig, error) {
	config := &entity.InstanceConfig{}
	row := s.Db.Where("name=?", configName).Find(config).RowsAffected
	if row != 1 {
		return nil, common.ServerErrorWithMsg("get instanceConfig(name=%s) error", configName)
	}
	return config, nil
}

func ListInstanceConfigs() ([]*entity.InstanceConfig, error) {
	instanceConfigs := make([]*entity.InstanceConfig, 0)
	err := s.Db.Find(&instanceConfigs).Error
	return instanceConfigs, err
}

func CreateInstanceConfigForAgentGroup(groupName string, configName string) error {
	removeConfig := entity.InstanceConfig{
		Name: configName,
	}
	agentGroup := entity.AgentGroup{
		Name: groupName,
	}
	err := s.Db.Model(&agentGroup).Association("InstanceConfigs").Append(&removeConfig)
	return common.SystemError(err)
}

//func CreateInstanceConfigForAgent(instanceId string, configName string) error {
//	agentPipelineConfig := entity.AgentPipelineConfig{
//		AgentInstanceId:    instanceId,
//		PipelineConfigName: configName,
//	}
//	err := s.Db.FirstOrCreate(&agentPipelineConfig).Error
//	return err
//}

//func CreateInstanceConfigForAgentInGroup(agentInstanceIds []string, configName string) error {
//	agentInstanceConfigs := make([]entity.AgentInstanceConfig, 0)
//	for _, instanceId := range agentInstanceIds {
//		agentInstanceConfig := entity.AgentInstanceConfig{
//			AgentInstanceId:    instanceId,
//			InstanceConfigName: configName,
//		}
//		agentInstanceConfigs = append(agentInstanceConfigs, agentInstanceConfig)
//	}
//
//	if len(agentInstanceConfigs) > 0 {
//		err := s.Db.Create(agentInstanceConfigs).Error
//		return common.SystemError(err)
//	}
//
//	return nil
//}

func DeleteInstanceConfigForAgentGroup(groupName string, configName string) error {
	removeConfig := entity.InstanceConfig{
		Name: configName,
	}
	agentGroup := entity.AgentGroup{
		Name: groupName,
	}
	err := s.Db.Model(&agentGroup).Association("InstanceConfigs").Delete(&removeConfig)
	return common.SystemError(err)
}

//func DeleteInstanceConfigForAgentInGroup(agentInstanceIds []string, configName string) error {
//	agentInstanceConfigs := make([]entity.AgentInstanceConfig, 0)
//	for _, instanceId := range agentInstanceIds {
//		agentInstanceConfig := entity.AgentInstanceConfig{
//			AgentInstanceId:    instanceId,
//			InstanceConfigName: configName,
//		}
//		agentInstanceConfigs = append(agentInstanceConfigs, agentInstanceConfig)
//	}
//	if len(agentInstanceConfigs) > 0 {
//		err := s.Db.Delete(agentInstanceConfigs).Error
//		return common.SystemError(err)
//	}
//	return nil
//}

func DeleteAllInstanceConfigAndAgent() {
	s.Db.Exec("TRUNCATE TABLE agent_instance_config")
}

//func ListAgentInstanceConfig() []*entity.AgentInstanceConfig {
//	agentInstanceConfigs := make([]*entity.AgentInstanceConfig, 0)
//	s.Db.Find(&agentInstanceConfigs)
//	return agentInstanceConfigs
//}
