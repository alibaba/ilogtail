package service

import (
	"config-server/common"
	"config-server/entity"
	"config-server/manager"
	"config-server/manager/state"
	proto "config-server/protov2"
	"config-server/repository"
	"config-server/utils"
	"time"
)

// CheckAgentExist  确认心跳，失败则下线对应的实例
func CheckAgentExist(timeLimit int64) {
	utils.TimedTask(timeLimit, func(timeLimitParam int64) {
		timeLimitNano := timeLimitParam * int64(time.Second)
		utils.ParallelProcessSlice[entity.Agent](manager.GetAllAgentsBasicInfo(),
			func(_ int, agentInfo entity.Agent) {
				manager.RemoveAgentNow(&agentInfo, timeLimitNano)
			})
	})
}

func HeartBeat(req *proto.HeartbeatRequest, res *proto.HeartbeatResponse) error {
	instanceId := req.InstanceId
	if instanceId == nil {
		return common.ValidateErrorWithMsg("required field instanceId could not be null")
	}

	var err error
	sequenceNum := req.SequenceNum

	rationality := manager.JudgeSequenceNumRationality(instanceId, sequenceNum)

	//假设数据库保存的sequenceNum=3,agent给的是10，
	//如果在判断rationality=false立即return,数据库中保存的一直是3，agent一直重传全部状态
	if !rationality {
		res.Flags = res.Flags | uint64(state.ReportFullState.Code)
	}

	currentHeatBeatTime := time.Now().UnixNano()
	req.Tags = manager.AddDefaultAgentGroup(req.Tags)
	agent := entity.ParseHeartBeatRequest2BasicAgent(req, currentHeatBeatTime)

	//Regardless of whether sequenceN is legal or not, we should keep the basic information of the agent (sequenceNum, instanceId, capabilities, flags, etc.)
	err = manager.CreateOrUpdateAgentBasicInfo(agent)
	if err != nil {
		return common.SystemError(err)
	}

	//如果req未设置fullState,agent不会上传configStatus
	err = state.HandleRequestFlags(req, res)
	if err != nil {
		return common.SystemError(err)
	}

	err = state.HandleAgentCapabilities(req, res)
	if err != nil {
		return common.SystemError(err)
	}

	return nil
}

// 心跳检测中的tag只是为了获取某个组的配置，而不是让某个agent加入到某个组

func FetchPipelineConfigDetail(req *proto.FetchConfigRequest, res *proto.FetchConfigResponse) error {
	instanceId := req.InstanceId
	if instanceId == nil {
		return common.ValidateErrorWithMsg("required field instanceId could not be null")
	}

	var err error
	strInstanceId := string(instanceId)

	//需要更新配置状态
	err = manager.SavePipelineConfigStatus(req.ReqConfigs, strInstanceId)
	if err != nil {
		return common.SystemError(err)
	}

	configNames := utils.Map(req.ReqConfigs, func(info *proto.ConfigInfo) string {
		return info.Name
	})
	pipelineConfigUpdates, err := repository.GetPipelineConfigByNames(configNames...)
	if err != nil {
		return err
	}

	res.ConfigDetails = utils.Map(pipelineConfigUpdates, func(config *entity.PipelineConfig) *proto.ConfigDetail {
		return config.Parse2ProtoPipelineConfigDetail(true)
	})
	return nil
}

func FetchInstanceConfigDetail(req *proto.FetchConfigRequest, res *proto.FetchConfigResponse) error {
	instanceId := req.InstanceId
	if instanceId == nil {
		return common.ValidateErrorWithMsg("required field instanceId could not be null")
	}

	var err error
	strInstanceId := string(instanceId)

	//需要更新配置状态
	err = manager.SaveInstanceConfigStatus(req.ReqConfigs, strInstanceId)
	if err != nil {
		return common.SystemError(err)
	}

	configNames := utils.Map(req.ReqConfigs, func(info *proto.ConfigInfo) string {
		return info.Name
	})
	instanceConfigUpdates, err := repository.GetInstanceConfigByNames(configNames...)
	if err != nil {
		return err
	}

	res.ConfigDetails = utils.Map(instanceConfigUpdates, func(config *entity.InstanceConfig) *proto.ConfigDetail {
		return config.Parse2ProtoInstanceConfigDetail(true)
	})
	return nil
}

func ListAgentsInGroup(req *proto.ListAgentsRequest, res *proto.ListAgentsResponse) error {
	groupName := req.GroupName
	if groupName == "" {
		return common.ValidateErrorWithMsg("required fields groupName could not be null")
	}
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
