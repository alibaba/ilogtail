package service

import (
	"config-server2/internal/common"
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/entity"
	manager2 "config-server2/internal/server_agent/manager"
	flag "config-server2/internal/server_agent/manager/flag"
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
			utils.ParallelProcessSlice[entity.Agent](manager2.GetAllAgentsBasicInfo(),
				func(_ int, agentInfo entity.Agent) {
					manager2.RemoveAgentNow(&agentInfo, timeLimitNano)
				})
		}
	}
}

func HeartBeat(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	var err error
	instanceId := req.InstanceId
	sequenceNum := req.SequenceNum
	//fmt.Println("sequenceNum:", sequenceNum)
	if utils.AllEmpty(req.RequestId, instanceId) {
		log.Print("required fields(requestId,instanceId) are null")
		return common.ValidateErrorWithMsg("required fields(requestId,instanceId)could not be null")
	}
	res.RequestId = req.RequestId

	rationality := manager2.JudgeSequenceNumRationality(instanceId, sequenceNum)

	//假设数据库保存的sequenceNum=3,agent给的是10，
	//如果在判断rationality=false立即return,数据库中保存的一直是3，agent一直重传全部状态
	if !rationality {
		res.Flags = res.Flags | uint64(flag.ResponseMap[flag.ReportFullState].Code)
	}

	currentHeatBeatTime := time.Now().UnixNano()
	agent := entity.HeartBeatRequestParse2BasicAgent(req, currentHeatBeatTime)
	//Regardless of whether sequenceN is legal or not, we should keep the basic information of the agent (sequenceNum, instanceId, capabilities, flags, etc.)

	err = manager2.CreateOrUpdateAgentBasicInfo(agent)
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

	return nil
}

func FetchPipelineConfigDetail(req *proto.FetchConfigRequest, res *proto.FetchConfigResponse) error {
	instanceId := req.InstanceId
	var err error
	if utils.AllEmpty(req.RequestId, instanceId) {
		log.Print("required fields(requestId,instanceId) are null")
		return common.ValidateErrorWithMsg("required fields(requestId,instanceId)could not be null")
	}

	res.RequestId = req.RequestId
	strInstanceId := string(instanceId)

	//创建或更新pipelineConfig的status and message
	agentPipelineConfigs := entity.ProtoConfigInfoParse2AgentPipelineConfig(strInstanceId, req.ReqConfigs)
	err = manager2.CreateOrUpdateAgentPipelineConfigs(agentPipelineConfigs)
	if err != nil {
		return err
	}

	//返回pipelineConfigDetail
	pipelineConfigUpdates, err := manager2.GetPipelineConfigs(strInstanceId, true)
	if err != nil {
		return err
	}
	res.ConfigDetails = pipelineConfigUpdates
	//fmt.Printf("%+v", res)
	return nil
}

func FetchInstanceConfigDetail(req *proto.FetchConfigRequest, res *proto.FetchConfigResponse) error {
	instanceId := req.InstanceId
	var err error
	if utils.AllEmpty(req.RequestId, instanceId) {
		log.Print("required fields(requestId,instanceId) are nil")
		return common.ValidateErrorWithMsg("required fields(requestId,instanceId)could not be null")
	}

	res.RequestId = req.RequestId
	strInstanceId := string(instanceId)

	//Store in the agent_instance table
	agentInstanceConfigs := entity.ProtoConfigInfoParse2AgentInstanceConfig(strInstanceId, req.ReqConfigs)
	err = manager2.CreateOrUpdateAgentInstanceConfigs(agentInstanceConfigs)
	if err != nil {
		return err
	}

	//获取对应的configDetails
	instanceConfigUpdates, err := manager2.GetInstanceConfigs(strInstanceId, true)
	if err != nil {
		return err
	}
	res.ConfigDetails = instanceConfigUpdates
	return nil
}
