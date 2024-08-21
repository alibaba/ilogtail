package manager

import (
	"config-server2/internal/entity"
	"config-server2/internal/store"
)

var s = store.S

func JudgeSequenceNumRationality(instanceId []byte, sequenceNum uint64) bool {
	//最开始的心跳检测sequenceNum=0,所以sequenceNum=0是合法的
	var strInstanceId = string(instanceId)
	agent := s.GetAgentByiId(strInstanceId)
	//找不到数据的时候，同样说明是最开始的状态，应返回true
	if agent == nil {
		return true
	}
	if sequenceNum != agent.SequenceNum+1 {
		return false
	}
	return true
}

func CreateOrUpdateBasicAgent(agent *entity.Agent) error {
	var err error
	var ok bool
	if ok, err = s.HasAgentById(agent.InstanceId); ok {
		err = s.UpdateAgentById(agent)
	} else {
		err = s.CreateBasicAgent(agent)
	}
	return err
}
