package service

import (
	"config-server2/internal/common"
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/entity"
	"config-server2/internal/manager"
	"config-server2/internal/manager/flag"
	"config-server2/internal/store"
	"config-server2/internal/utils"
	"log"
	"time"
)

var s = store.S

// CheckAgentExist  确认心跳，失败则下线对应的实例
func CheckAgentExist(timeLimit int64) {
	if timeLimit <= 0 {
		panic("timeLimit 不能小于等于0")
	}
	timeLimitNano := timeLimit * int64(time.Second)

	ticker := time.NewTicker(time.Duration(timeLimitNano))
	defer ticker.Stop()

	for {
		select {
		case <-ticker.C:
			for _, agentInfo := range s.GetAllAgentsBasicInfo() {
				nowTime := time.Now().UnixNano()
				if nowTime-agentInfo.LastHeartBeatTime >= timeLimitNano {
					err := s.RemoveAgentById(agentInfo.InstanceId)
					if err != nil {
						log.Println(err)
					}
				}
			}
		}
	}

}

func HeartBeat(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	var err error
	instanceId := req.InstanceId
	sequenceNum := req.SequenceNum
	//fmt.Println("sequenceNum:", sequenceNum)
	if utils.AllEmpty(req.RequestId, instanceId) {
		log.Print("required fields(requestId,instanceId) are nil")
		return common.ValidateErrorWithMsg("required fields(requestId,instanceId)could not be nil")
	}
	res.RequestId = req.RequestId

	rationality := manager.JudgeSequenceNumRationality(instanceId, sequenceNum)

	//假设数据库保存的sequenceNum=3,agent给的是10，
	//如果在判断rationality=false立即return,数据库中保存的一直是3，agent一直重传全部状态
	if !rationality {
		res.Flags = res.Flags | uint64(flag.ResponseMap[flag.ReportFullState].Code)
	}

	currentHeatBeatTime := time.Now().UnixNano()
	agent := entity.HeartBeatRequestParse2BasicAgent(req, currentHeatBeatTime)
	//Regardless of whether sequenceN is legal or not, we should keep the basic information of the agent (seNum, instanceId, capabilities, flags, etc.)
	err = manager.CreateOrUpdateBasicAgent(agent)
	if err != nil {
		return err
	}

	//如果req未设置fullState,agent会不会上传其他的configStatus
	err = flag.HandleRequestFlags(req, res)
	if err != nil {
		return err
	}
	err = flag.HandleResponseFlags(req, res)
	if err != nil {
		return err
	}
	//fmt.Printf("%+v\n", res)
	return nil
}

func FetchPipelineConfigDetail(req *proto.FetchConfigRequest, res *proto.FetchConfigResponse) error {
	instanceId := req.InstanceId
	var err error
	if utils.AllEmpty(req.RequestId, instanceId) {
		log.Print("required fields(requestId,instanceId) are nil")
		return common.ValidateErrorWithMsg("required fields(requestId,instanceId)could not be null")
	}

	res.RequestId = req.RequestId
	strInstanceId := string(instanceId)

	agentPipelineConfigs := entity.ProtoConfigInfoParse2AgentPipelineConfig(strInstanceId, req.ReqConfigs)
	err = manager.CreateOrUpdateAgentPipelineConfig(agentPipelineConfigs)
	if err != nil {
		return err
	}
	pipelineConfigUpdates, err := manager.GetPipelineConfig(strInstanceId, true)
	if err != nil {
		return err
	}
	res.ConfigDetails = pipelineConfigUpdates
	//fmt.Printf("%+v", res)
	return nil
}

func FetchProcessConfigDetail(req *proto.FetchConfigRequest, res *proto.FetchConfigResponse) error {
	instanceId := req.InstanceId
	var err error
	if utils.AllEmpty(req.RequestId, instanceId) {
		log.Print("required fields(requestId,instanceId) are nil")
		return common.ValidateErrorWithMsg("required fields(requestId,instanceId)could not be null")
	}

	res.RequestId = req.RequestId
	strInstanceId := string(instanceId)

	//Store in the agent_process table
	agentProcessConfigs := entity.ProtoConfigInfoParse2AgentProcessConfig(strInstanceId, req.ReqConfigs)
	err = manager.CreateOrUpdateAgentProcessConfig(agentProcessConfigs)
	if err != nil {
		return err
	}

	//获取对应的configDetails
	processConfigUpdates, err := manager.GetProcessConfig(strInstanceId, true)
	if err != nil {
		return err
	}
	res.ConfigDetails = processConfigUpdates
	return nil
}
