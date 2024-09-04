package service

import (
	"config-server2/internal/common"
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/entity"
	"config-server2/internal/repository"
	"log"
)

func CreateAgentGroup(req *proto.CreateAgentGroupRequest, res *proto.CreateAgentGroupResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	agentGroup := req.AgentGroup
	if agentGroup == nil {
		log.Print("required field agentGroup is null")
		return common.ValidateErrorWithMsg("required field agentGroup could not be null")
	}

	res.RequestId = requestId
	group := entity.ParseProtoAgentGroupTag2AgentGroup(agentGroup)
	err := repository.CreateAgentGroup(group)
	return err
}

func UpdateAgentGroup(req *proto.UpdateAgentGroupRequest, res *proto.UpdateAgentGroupResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	agentGroup := req.AgentGroup
	if agentGroup == nil {
		log.Print("required field agentGroup is null")
		return common.ValidateErrorWithMsg("required field agentGroup could not be null")
	}

	res.RequestId = requestId
	group := entity.ParseProtoAgentGroupTag2AgentGroup(agentGroup)
	err := repository.UpdateAgentGroup(group)
	return err
}

func DeleteAgentGroup(req *proto.DeleteAgentGroupRequest, res *proto.DeleteAgentGroupResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	agentGroupName := req.GroupName
	if agentGroupName == "" {
		log.Print("required field groupName is null")
		return common.ValidateErrorWithMsg("required field groupName could not be null")
	}
	res.RequestId = requestId
	err := repository.DeleteAgentGroup(agentGroupName)
	return err
}

func GetAgentGroup(req *proto.GetAgentGroupRequest, res *proto.GetAgentGroupResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	agentGroupName := req.GroupName
	if agentGroupName == "" {
		log.Print("required field groupName is null")
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
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	res.RequestId = requestId
	agentGroups, err := repository.GetAllAgentGroup()
	if err != nil {
		return err
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
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
	configName := req.ConfigName
	if configName == "" {
		log.Print("required fields configName is null")
		return common.ValidateErrorWithMsg("required fields configName could not be null")
	}

	res.RequestId = requestId

	groupNames, err := repository.GetAppliedAgentGroupForPipelineConfigName(configName)
	if err != nil {
		return err
	}
	res.AgentGroupNames = groupNames
	return nil
}

func GetAppliedAgentGroupsForInstanceConfigName(req *proto.GetAppliedAgentGroupsRequest, res *proto.GetAppliedAgentGroupsResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
	configName := req.ConfigName
	if configName == "" {
		log.Print("required fields configName is null")
		return common.ValidateErrorWithMsg("required fields configName could not be null")
	}

	res.RequestId = requestId

	groupNames, err := repository.GetAppliedAgentGroupForInstanceConfigName(configName)
	if err != nil {
		return err
	}
	res.AgentGroupNames = groupNames
	return nil
}
