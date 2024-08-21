package manager

import (
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/entity"
	"errors"
	"gorm.io/gorm"
)

func CreateOrUpdateAgentProcessConfig(agentProcessConfig []*entity.AgentProcessConfig) error {
	var err error
	for _, config := range agentProcessConfig {
		err = s.DB.Where("agent_instance_id=? and process_config_name =?",
			config.AgentInstanceId, config.ProcessConfigName).Take(config).Error
		if errors.Is(err, gorm.ErrRecordNotFound) {
			err = s.DB.Create(config).Error
		} else if err != nil {
			return err
		} else {
			err = s.DB.Model(&entity.AgentProcessConfig{}).
				Where("agent_instance_id=? and process_config_name=?", config.AgentInstanceId, config.ProcessConfigName).Updates(config).Error
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

func GetProcessConfig(instanceId string, isContainDetail bool) ([]*proto.ConfigDetail, error) {
	var err error
	agent := &entity.Agent{InstanceId: instanceId}
	err = s.DB.Preload("ProcessConfigs").Find(agent).Error
	if err != nil {
		return nil, err
	}

	processConfigUpdates := make([]*proto.ConfigDetail, 0)
	for _, config := range agent.ProcessConfigs {
		detail := config.Parse2ProtoProcessConfigDetail(isContainDetail)
		processConfigUpdates = append(processConfigUpdates, detail)
	}
	return processConfigUpdates, err
}
