package service

import (
	"config-server2/internal/common"
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/entity"
	manager "config-server2/internal/manager"
	flag "config-server2/internal/manager/flag"
	"config-server2/internal/repository"
	"config-server2/internal/store"
	"config-server2/internal/utils"
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
			utils.ParallelProcessSlice[entity.Agent](manager.GetAllAgentsBasicInfo(),
				func(_ int, agentInfo entity.Agent) {
					manager.RemoveAgentNow(&agentInfo, timeLimitNano)
				})
		}
	}
}

func HeartBeat(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	instanceId := req.InstanceId
	if instanceId == nil {
		return common.ValidateErrorWithMsg("required field instanceId could not be null")
	}

	var err error
	res.RequestId = requestId
	sequenceNum := req.SequenceNum

	rationality := manager.JudgeSequenceNumRationality(instanceId, sequenceNum)

	//假设数据库保存的sequenceNum=3,agent给的是10，
	//如果在判断rationality=false立即return,数据库中保存的一直是3，agent一直重传全部状态
	if !rationality {
		res.Flags = res.Flags | uint64(flag.ResponseMap[flag.ReportFullState].Code)
	}

	currentHeatBeatTime := time.Now().UnixNano()
	agent := entity.ParseHeartBeatRequest2BasicAgent(req, currentHeatBeatTime)
	agent.Tags = manager.AddDefaultAgentGroup(agent.Tags)

	//Regardless of whether sequenceN is legal or not, we should keep the basic information of the agent (sequenceNum, instanceId, capabilities, flags, etc.)
	err = manager.CreateOrUpdateAgentBasicInfo(agent)
	if err != nil {
		return common.SystemError(err)
	}

	//如果req未设置fullState,agent会不会上传其他的configStatus
	err = flag.HandleRequestFlags(req, res)
	if err != nil {
		return common.SystemError(err)
	}

	err = flag.HandleResponseFlags(req, res)
	if err != nil {
		return common.SystemError(err)
	}

	return nil
}

func FetchPipelineConfigDetail(req *proto.FetchConfigRequest, res *proto.FetchConfigResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	instanceId := req.InstanceId
	if instanceId == nil {
		return common.ValidateErrorWithMsg("required field instanceId could not be null")
	}

	var err error
	res.RequestId = requestId
	strInstanceId := string(instanceId)

	//创建或更新pipelineConfig的status and message
	agentPipelineConfigs := make([]*entity.AgentPipelineConfig, 0)
	for _, reqConfig := range req.ReqConfigs {
		agentPipelineConfig := entity.ParseProtoConfigInfo2AgentPipelineConfig(strInstanceId, reqConfig)
		agentPipelineConfigs = append(agentPipelineConfigs, agentPipelineConfig)
	}

	err = manager.CreateOrUpdateAgentPipelineConfigs(agentPipelineConfigs)
	if err != nil {
		return common.SystemError(err)
	}

	//返回pipelineConfigDetail
	pipelineConfigUpdates, err := manager.GetPipelineConfigs(strInstanceId, true)
	if err != nil {
		return common.SystemError(err)
	}
	res.ConfigDetails = pipelineConfigUpdates
	//fmt.PrintfRed("%+v", res)
	return nil
}

func FetchInstanceConfigDetail(req *proto.FetchConfigRequest, res *proto.FetchConfigResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	instanceId := req.InstanceId
	if instanceId == nil {
		return common.ValidateErrorWithMsg("required field instanceId could not be null")
	}

	var err error
	res.RequestId = requestId
	strInstanceId := string(instanceId)

	//Store in the agent_instance table

	agentInstanceConfigs := make([]*entity.AgentInstanceConfig, 0)
	for _, reqConfig := range req.ReqConfigs {
		agentInstanceConfig := entity.ParseProtoConfigInfo2AgentInstanceConfig(strInstanceId, reqConfig)
		agentInstanceConfigs = append(agentInstanceConfigs, agentInstanceConfig)
	}

	err = manager.CreateOrUpdateAgentInstanceConfigs(agentInstanceConfigs)
	if err != nil {
		return common.SystemError(err)
	}

	//获取对应的configDetails
	instanceConfigUpdates, err := manager.GetInstanceConfigs(strInstanceId, true)
	if err != nil {
		return common.SystemError(err)
	}
	res.ConfigDetails = instanceConfigUpdates
	return nil
}

func ListAgentsInGroup(req *proto.ListAgentsRequest, res *proto.ListAgentsResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		return common.ValidateErrorWithMsg("required fields requestId is null")
	}
	groupName := req.GroupName
	if groupName == "" {
		return common.ValidateErrorWithMsg("required fields groupName is null")
	}
	res.RequestId = req.RequestId
	agents, err := repository.ListAgentsByGroupName(groupName)
	if err != nil {
		return common.SystemError(err)
	}
	protoAgents := make([]*proto.Agent, 0)
	for _, agent := range agents {
		protoAgents = append(protoAgents, (*agent).Parse2Proto())
	}
	res.Agents = protoAgents
	return nil
}
