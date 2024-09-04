package service

import (
	"config-server2/internal/common"
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/entity"
	"config-server2/internal/repository"
	"log"
)

func CreatePipelineConfig(req *proto.CreateConfigRequest, res *proto.CreateConfigResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	configDetail := req.ConfigDetail
	if configDetail.Name == "" {
		log.Print("required field configName is null")
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}

	if configDetail.Version == 0 {
		log.Print("required field version is null")
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}
	res.RequestId = requestId
	pipelineConfig := entity.ParseProtoPipelineConfig2PipelineConfig(configDetail)
	err := repository.CreatePipelineConfig(pipelineConfig)
	return err

}

func UpdatePipelineConfig(req *proto.UpdateConfigRequest, res *proto.UpdateConfigResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	configDetail := req.ConfigDetail
	if configDetail.Name == "" {
		log.Print("required field configName is null")
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}

	if configDetail.Version == 0 {
		log.Print("required field version is null")
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}
	res.RequestId = requestId
	pipelineConfig := entity.ParseProtoPipelineConfig2PipelineConfig(configDetail)
	err := repository.UpdatePipelineConfig(pipelineConfig)
	return err
}

func DeletePipelineConfig(req *proto.DeleteConfigRequest, res *proto.DeleteConfigResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	configName := req.ConfigName
	if configName == "" {
		log.Print("required field configName is null")
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}
	res.RequestId = requestId
	err := repository.DeletePipelineConfig(configName)
	return err
}

func GetPipelineConfig(req *proto.GetConfigRequest, res *proto.GetConfigResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}

	configName := req.ConfigName
	if configName == "" {
		log.Print("required field configName is null")
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}
	res.RequestId = requestId
	pipelineConfig, err := repository.GetPipelineConfig(configName)
	if err != nil {
		return err
	}
	if pipelineConfig != nil {
		res.ConfigDetail = pipelineConfig.Parse2ProtoPipelineConfigDetail(true)
	}
	return err
}

func ListPipelineConfigs(req *proto.ListConfigsRequest, res *proto.ListConfigsResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
	res.RequestId = requestId
	pipelineConfigs, err := repository.ListPipelineConfigs()
	if err != nil {
		return err
	}

	for _, pipelineConfig := range pipelineConfigs {
		res.ConfigDetails = append(res.ConfigDetails, pipelineConfig.Parse2ProtoPipelineConfigDetail(true))
	}
	return nil
}

func ApplyPipelineConfigToAgentGroup(req *proto.ApplyConfigToAgentGroupRequest, res *proto.ApplyConfigToAgentGroupResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
	groupName := req.GroupName
	if groupName == "" {
		log.Print("required fields groupName is null")
		return common.ValidateErrorWithMsg("required fields groupName could not be null")
	}
	configName := req.ConfigName
	if configName == "" {
		log.Print("required fields configName is null")
		return common.ValidateErrorWithMsg("required fields configName could not be null")
	}
	res.RequestId = requestId
	var err error
	err = repository.CreatePipelineConfigForAgentGroup(groupName, configName)
	if err != nil {
		return err
	}

	agents, err := repository.ListAgentsByGroupName(groupName)
	if err != nil {
		return err
	}

	agentInstanceIds := make([]string, 0)
	for _, agent := range agents {
		agentInstanceIds = append(agentInstanceIds, agent.InstanceId)
	}

	return repository.CreatePipelineConfigForAgentInGroup(agentInstanceIds, configName)

}

func RemovePipelineConfigFromAgentGroup(req *proto.RemoveConfigFromAgentGroupRequest, res *proto.RemoveConfigFromAgentGroupResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
	groupName := req.GroupName
	if groupName == "" {
		log.Print("required fields groupName is null")
		return common.ValidateErrorWithMsg("required fields groupName could not be null")
	}
	configName := req.ConfigName
	if configName == "" {
		log.Print("required fields configName is null")
		return common.ValidateErrorWithMsg("required fields configName could not be null")
	}
	res.RequestId = requestId

	err := repository.DeletePipelineConfigForAgentGroup(groupName, configName)
	if err != nil {
		return err
	}

	agents, err := repository.ListAgentsByGroupName(groupName)
	if err != nil {
		return err
	}

	agentInstanceIds := make([]string, 0)
	for _, agent := range agents {
		agentInstanceIds = append(agentInstanceIds, agent.InstanceId)
	}

	return repository.DeletePipelineConfigForAgentInGroup(agentInstanceIds, configName)
}

func GetAppliedPipelineConfigsForAgentGroup(req *proto.GetAppliedConfigsForAgentGroupRequest, res *proto.GetAppliedConfigsForAgentGroupResponse) error {
	requestId := req.RequestId
	if requestId == nil {
		log.Print("required fields requestId is null")
		return common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
	groupName := req.GroupName
	if groupName == "" {
		log.Print("required fields groupName is null")
		return common.ValidateErrorWithMsg("required fields groupName could not be null")
	}
	res.RequestId = requestId

	agentGroupDetail, err := repository.GetAgentGroupDetail(groupName, true, false)
	if err != nil {
		return err
	}
	configNames := make([]string, 0)
	for _, config := range agentGroupDetail.PipelineConfigs {
		configNames = append(configNames, config.Name)
	}

	res.ConfigNames = configNames
	return nil
}
