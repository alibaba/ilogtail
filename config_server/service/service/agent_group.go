package service

import (
	"config-server/common"
	"config-server/entity"
	"config-server/manager"
	proto "config-server/protov2"
	"config-server/repository"
	"config-server/utils"
)

// AppliedOrRemoveConfigForAgentGroup 定期检查在group中的agent与config的关系是否符合group与config的关系
func AppliedOrRemoveConfigForAgentGroup(timeLimit int64) {
	utils.TimedTask(timeLimit, func(int64) {
		agentGroupDetails, err := repository.GetAllAgentGroupDetail(true, true, true)
		if err != nil {
			panic(err)
		}
		utils.ParallelProcessTask(agentGroupDetails,
			manager.AppliedOrRemovePipelineConfigForAgentGroup,
			manager.AppliedOrRemoveInstanceConfigForAgentGroup)
	})
}

func CreateAgentGroup(req *proto.CreateAgentGroupRequest, res *proto.CreateAgentGroupResponse) error {
	agentGroup := req.AgentGroup
	if agentGroup == nil {
		return common.ValidateErrorWithMsg("required field agentGroup could not be null")
	}
	if utils.IsEmptyOrWhitespace(agentGroup.Name) {
		return common.ValidateErrorWithMsg("required field agentGroupName could not be null")
	}

	group := entity.ParseProtoAgentGroupTag2AgentGroup(agentGroup)
	err := repository.CreateAgentGroup(group)
	return common.SystemError(err)
}

func UpdateAgentGroup(req *proto.UpdateAgentGroupRequest, res *proto.UpdateAgentGroupResponse) error {
	agentGroup := req.AgentGroup
	if agentGroup == nil {
		return common.ValidateErrorWithMsg("required field agentGroup could not be null")
	}
	group := entity.ParseProtoAgentGroupTag2AgentGroup(agentGroup)
	err := repository.UpdateAgentGroup(group)
	return common.SystemError(err)
}

func DeleteAgentGroup(req *proto.DeleteAgentGroupRequest, res *proto.DeleteAgentGroupResponse) error {
	agentGroupName := req.GroupName
	if agentGroupName == "" {
		return common.ValidateErrorWithMsg("required field groupName could not be null")
	}
	if req.GroupName == entity.AgentGroupDefaultValue {
		return common.ServerErrorWithMsg("%s can not be deleted", entity.AgentGroupDefaultValue)
	}
	err := repository.DeleteAgentGroup(agentGroupName)
	return common.SystemError(err)
}

func GetAgentGroup(req *proto.GetAgentGroupRequest, res *proto.GetAgentGroupResponse) error {
	agentGroupName := req.GroupName
	if agentGroupName == "" {
		return common.ValidateErrorWithMsg("required field groupName could not be null")
	}

	agentGroup, err := repository.GetAgentGroupDetail(agentGroupName, false, false)
	if err != nil {
		return common.SystemError(err)
	}
	res.AgentGroup = agentGroup.Parse2ProtoAgentGroupTag()
	return nil
}

func ListAgentGroups(req *proto.ListAgentGroupsRequest, res *proto.ListAgentGroupsResponse) error {
	agentGroups, err := repository.GetAllAgentGroup()
	if err != nil {
		return common.SystemError(err)
	}
	protoAgentGroups := make([]*proto.AgentGroupTag, 0)
	for _, agentGroup := range agentGroups {
		protoAgentGroups = append(protoAgentGroups, agentGroup.Parse2ProtoAgentGroupTag())
	}
	res.AgentGroups = protoAgentGroups
	return nil
}

func GetAppliedAgentGroupsForPipelineConfigName(req *proto.GetAppliedAgentGroupsRequest, res *proto.GetAppliedAgentGroupsResponse) error {
	configName := req.ConfigName
	if configName == "" {
		return common.ValidateErrorWithMsg("required fields configName could not be null")
	}

	groupNames, err := repository.GetAppliedAgentGroupForPipelineConfigName(configName)
	if err != nil {
		return common.SystemError(err)
	}
	res.AgentGroupNames = groupNames
	return nil
}

func GetAppliedAgentGroupsForInstanceConfigName(req *proto.GetAppliedAgentGroupsRequest, res *proto.GetAppliedAgentGroupsResponse) error {
	configName := req.ConfigName
	if configName == "" {
		return common.ValidateErrorWithMsg("required fields configName could not be null")
	}

	groupNames, err := repository.GetAppliedAgentGroupForInstanceConfigName(configName)
	if err != nil {
		return common.SystemError(err)
	}
	res.AgentGroupNames = groupNames
	return nil
}
