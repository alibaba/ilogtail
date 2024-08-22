package manager

import (
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/entity"
	"errors"
	"gorm.io/gorm"
)

func CreateOrUpdateAgentInstanceConfig(agentInstanceConfig []*entity.AgentInstanceConfig) error {
	var err error
	for _, config := range agentInstanceConfig {
		err = s.DB.Where("agent_instance_id=? and instance_config_name =?",
			config.AgentInstanceId, config.InstanceConfigName).Take(config).Error
		if errors.Is(err, gorm.ErrRecordNotFound) {
			err = s.DB.Create(config).Error
		} else if err != nil {
			return err
		} else {
			err = s.DB.Model(&entity.AgentInstanceConfig{}).
				Where("agent_instance_id=? and instance_config_name=?", config.AgentInstanceId, config.InstanceConfigName).Updates(config).Error
			if err != nil {
				return err
			}
		}
	}
	return nil
}

func CreateOrUpdateAgentPipelineConfig(agentPipelineConfig []*entity.AgentPipelineConfig) error {
	var err error
	for _, config := range agentPipelineConfig {
		err = s.DB.Where("agent_instance_id=? and pipeline_config_name =?",
			config.AgentInstanceId, config.PipelineConfigName).Take(config).Error
		if errors.Is(err, gorm.ErrRecordNotFound) {
			err = s.DB.Create(config).Error
		} else if err != nil {
			return err
		} else {
			err = s.DB.Model(&entity.AgentPipelineConfig{}).
				Where("agent_instance_id=? and pipeline_config_name=?", config.AgentInstanceId, config.PipelineConfigName).Updates(config).Error
			if err != nil {
				return err
			}
		}
	}
	return nil
}

func GetPipelineConfig(instanceId string, isContainDetail bool) ([]*proto.ConfigDetail, error) {
	var err error
	agent := &entity.Agent{InstanceId: instanceId}
	err = s.DB.Preload("PipelineConfigs").Find(agent).Error
	if err != nil {
		return nil, err
	}

	pipelineConfigUpdates := make([]*proto.ConfigDetail, 0)
	for _, config := range agent.PipelineConfigs {
		detail := config.Parse2ProtoPipelineConfigDetail(isContainDetail)
		pipelineConfigUpdates = append(pipelineConfigUpdates, detail)
	}
	return pipelineConfigUpdates, nil
}

func GetInstanceConfig(instanceId string, isContainDetail bool) ([]*proto.ConfigDetail, error) {
	var err error
	agent := &entity.Agent{InstanceId: instanceId}
	err = s.DB.Preload("InstanceConfigs").Find(agent).Error
	if err != nil {
		return nil, err
	}

	instanceConfigUpdates := make([]*proto.ConfigDetail, 0)
	for _, config := range agent.InstanceConfigs {
		detail := config.Parse2ProtoInstanceConfigDetail(isContainDetail)
		instanceConfigUpdates = append(instanceConfigUpdates, detail)
	}
	return instanceConfigUpdates, err
}
