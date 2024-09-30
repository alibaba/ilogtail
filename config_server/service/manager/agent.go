package manager

import (
	"config-server/entity"
	"config-server/repository"
	"log"
	"time"
)

func JudgeSequenceNumRationality(instanceId []byte, sequenceNum uint64) bool {
	//最开始的心跳检测sequenceNum=0,所以sequenceNum=0是合法的
	var strInstanceId = string(instanceId)
	agent := repository.GetAgentByiId(strInstanceId)
	//找不到数据的时候，同样说明是最开始的状态，应返回true
	if agent == nil {
		return true
	}
	if sequenceNum != agent.SequenceNum+1 {
		return false
	}
	return true
}

func CreateOrUpdateAgentBasicInfo(agent *entity.Agent) error {
	conflictColumnNames := []string{"instance_id"}
	return repository.CreateOrUpdateAgentBasicInfo(conflictColumnNames, agent)
}

func GetAllAgentsBasicInfo() []entity.Agent {
	return repository.GetAllAgents(false, false)
}

func RemoveAgentNow(agentInfo *entity.Agent, timeLimitNano int64) {
	nowTime := time.Now().UnixNano()
	if nowTime-agentInfo.LastHeartBeatTime >= timeLimitNano {
		err := repository.RemoveAgentById(agentInfo.InstanceId)
		if err != nil {
			log.Printf("remove agent (id=%s) error %s", agentInfo.InstanceId, err)
		} else {
			log.Printf("remove agent (id=%s) success", agentInfo.InstanceId)
		}
	}
}
