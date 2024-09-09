package service

import (
	"config-server2/internal/common"
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/entity"
	"config-server2/internal/repository"
)

func CreateAgentGroup(req *proto.CreateAgentGroupRequest, res *proto.CreateAgentGroupResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	agentGroup := req.AgentGroup
	if agentGroup == nil {
		return common.ValidateErrorWithMsg("required field agentGroup could not be null")
	}

	res.RequestId = requestId
	group := entity.ParseProtoAgentGroupTag2AgentGroup(agentGroup)
	err := repository.CreateAgentGroup(group)
	return common.SystemError(err)
}

func UpdateAgentGroup(req *proto.UpdateAgentGroupRequest, res *proto.UpdateAgentGroupResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	agentGroup := req.AgentGroup
	if agentGroup == nil {
		return common.ValidateErrorWithMsg("required field agentGroup could not be null")
	}

	res.RequestId = requestId
	group := entity.ParseProtoAgentGroupTag2AgentGroup(agentGroup)
	err := repository.UpdateAgentGroup(group)
	return common.SystemError(err)
}

func DeleteAgentGroup(req *proto.DeleteAgentGroupRequest, res *proto.DeleteAgentGroupResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	agentGroupName := req.GroupName
	if agentGroupName == "" {
		return common.ValidateErrorWithMsg("required field groupName could not be null")
	}
	res.RequestId = requestId
	if req.GroupName == entity.AgentGroupDefaultValue {
		return common.ServerErrorWithMsg("%s can not be deleted", entity.AgentGroupDefaultValue)
	}
	err := repository.DeleteAgentGroup(agentGroupName)
	return common.SystemError(err)
}

func GetAgentGroup(req *proto.GetAgentGroupRequest, res *proto.GetAgentGroupResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	agentGroupName := req.GroupName
	if agentGroupName == "" {
		return common.ValidateErrorWithMsg("required field groupName could not be null")
	}

	res.RequestId = requestId
	agentGroup, err := repository.GetAgentGroupDetail(agentGroupName, false, false)
	if err != nil {
		return nil
	}
	res.AgentGroup = agentGroup.Parse2ProtoAgentGroupTag()
	return nil
}

func ListAgentGroups(req *proto.ListAgentGroupsRequest, res *proto.ListAgentGroupsResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	res.RequestId = requestId
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
	requestId := req.RequestId
	if requestId == nil {
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
	configName := req.ConfigName
	if configName == "" {
		return common.ValidateErrorWithMsg("required fields configName could not be null")
	}

	res.RequestId = requestId

	groupNames, err := repository.GetAppliedAgentGroupForPipelineConfigName(configName)
	if err != nil {
		return common.SystemError(err)
	}
	res.AgentGroupNames = groupNames
	return nil
}

func GetAppliedAgentGroupsForInstanceConfigName(req *proto.GetAppliedAgentGroupsRequest, res *proto.GetAppliedAgentGroupsResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
	configName := req.ConfigName
	if configName == "" {
		return common.ValidateErrorWithMsg("required fields configName could not be null")
	}

	res.RequestId = requestId

	groupNames, err := repository.GetAppliedAgentGroupForInstanceConfigName(configName)
	if err != nil {
		return common.SystemError(err)
	}
	res.AgentGroupNames = groupNames
	return nil
}
