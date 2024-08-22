package store

import (
	"config-server2/internal/common"
	"config-server2/internal/entity"
	"gorm.io/gorm"
	"log"
)

func (s *GormStore) CreateBasicAgent(agent *entity.Agent) error {
	if agent.InstanceId == "" {
		return common.ValidateErrorWithMsg("InstanceId can not be null")
	}
	err := s.DB.Create(agent).Error
	return err
}

func (s *GormStore) GetAgentByiId(instanceId string) *entity.Agent {
	var agentInfo = new(entity.Agent)
	row := s.DB.Where("instance_id=?", instanceId).Find(agentInfo).RowsAffected
	if row == 1 {
		return agentInfo
	}
	return nil
}

func (s *GormStore) HasAgentById(instanceId string) (bool, error) {
	var count int64
	s.DB.Model(&entity.Agent{}).Where("instance_id=?", instanceId).Count(&count)
	return count == 1, nil
}

func (s *GormStore) GetAllAgentsBasicInfo() []entity.Agent {
	var agentInfoList []entity.Agent
	s.DB.Find(&agentInfoList)
	return agentInfoList
}

func (s *GormStore) RemoveAgentById(instanceId string) error {
	var tx *gorm.DB

	tx = s.DB.Where("instance_id=?", instanceId).Delete(&entity.Agent{})
	if tx.Error != nil {
		log.Print(tx.Error)
	} else if tx.RowsAffected != 1 {
		return common.ServerErrorWithMsg("Agent failed to delete record %s", instanceId)
	}

	s.DB.Where("agent_instance_id=?", instanceId).Delete(&entity.AgentPipelineConfig{})
	s.DB.Where("agent_instance_id=?", instanceId).Delete(&entity.AgentInstanceConfig{})
	return nil
}

func (s *GormStore) UpdateAgentById(agent *entity.Agent) error {
	err := s.DB.Model(agent).Where("instance_id=?", agent.InstanceId).Updates(*agent).Error
	return err
}

func (s *GormStore) GetPipelineConfigDetailByName(configName string) error {
	configDetail := new(entity.PipelineConfig)
	err := s.DB.Where("name=?", configName).Take(configDetail).Error
	return err
}
