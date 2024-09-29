package service

import (
	"config-server/internal/common"
	"config-server/internal/entity"
	"config-server/internal/manager"
	"config-server/internal/protov2"
	"config-server/internal/repository"
	"config-server/internal/utils"
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

func CreateAgentGroup(req *protov2.CreateAgentGroupRequest, res *protov2.CreateAgentGroupResponse) error {
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

func UpdateAgentGroup(req *protov2.UpdateAgentGroupRequest, res *protov2.UpdateAgentGroupResponse) error {
	agentGroup := req.AgentGroup
	if agentGroup == nil {
		return common.ValidateErrorWithMsg("required field agentGroup could not be null")
	}
	group := entity.ParseProtoAgentGroupTag2AgentGroup(agentGroup)
	err := repository.UpdateAgentGroup(group)
	return common.SystemError(err)
}

func DeleteAgentGroup(req *protov2.DeleteAgentGroupRequest, res *protov2.DeleteAgentGroupResponse) error {
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

func GetAgentGroup(req *protov2.GetAgentGroupRequest, res *protov2.GetAgentGroupResponse) error {
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

func ListAgentGroups(req *protov2.ListAgentGroupsRequest, res *protov2.ListAgentGroupsResponse) error {
	agentGroups, err := repository.GetAllAgentGroup()
	if err != nil {
		return common.SystemError(err)
	}
	protoAgentGroups := make([]*protov2.AgentGroupTag, 0)
	for _, agentGroup := range agentGroups {
		protoAgentGroups = append(protoAgentGroups, agentGroup.Parse2ProtoAgentGroupTag())
	}
	res.AgentGroups = protoAgentGroups
	return nil
}

func GetAppliedAgentGroupsForPipelineConfigName(req *protov2.GetAppliedAgentGroupsRequest, res *protov2.GetAppliedAgentGroupsResponse) error {
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

func GetAppliedAgentGroupsForInstanceConfigName(req *protov2.GetAppliedAgentGroupsRequest, res *protov2.GetAppliedAgentGroupsResponse) error {
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
