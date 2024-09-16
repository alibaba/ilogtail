package service

import (
	"config-server2/internal/common"
	"config-server2/internal/entity"
	proto "config-server2/internal/protov2"
	"config-server2/internal/repository"
	"config-server2/internal/utils"
)

func CreatePipelineConfig(req *proto.CreateConfigRequest, res *proto.CreateConfigResponse) error {
	configDetail := req.ConfigDetail
	if utils.IsEmptyOrWhitespace(configDetail.Name) {
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}

	if configDetail.Version == 0 {
		return common.ValidateErrorWithMsg("required field version could not be null")
	}

	pipelineConfig := entity.ParseProtoPipelineConfig2PipelineConfig(configDetail)
	err := repository.CreatePipelineConfig(pipelineConfig)
	return common.SystemError(err)

}

func UpdatePipelineConfig(req *proto.UpdateConfigRequest, res *proto.UpdateConfigResponse) error {
	configDetail := req.ConfigDetail
	if configDetail.Name == "" {
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}

	if configDetail.Version == 0 {
		return common.ValidateErrorWithMsg("required field version could not be null")
	}
	pipelineConfig := entity.ParseProtoPipelineConfig2PipelineConfig(configDetail)
	err := repository.UpdatePipelineConfig(pipelineConfig)
	return common.SystemError(err)
}

func DeletePipelineConfig(req *proto.DeleteConfigRequest, res *proto.DeleteConfigResponse) error {
	configName := req.ConfigName
	if configName == "" {
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}
	err := repository.DeletePipelineConfig(configName)
	return common.SystemError(err)
}

func GetPipelineConfig(req *proto.GetConfigRequest, res *proto.GetConfigResponse) error {
	configName := req.ConfigName
	if configName == "" {
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}

	pipelineConfig, err := repository.GetPipelineConfig(configName)
	if err != nil {
		return common.SystemError(err)
	}
	if pipelineConfig != nil {
		res.ConfigDetail = pipelineConfig.Parse2ProtoPipelineConfigDetail(true)
	}
	return common.SystemError(err)
}

func ListPipelineConfigs(req *proto.ListConfigsRequest, res *proto.ListConfigsResponse) error {
	pipelineConfigs, err := repository.ListPipelineConfigs()
	if err != nil {
		return common.SystemError(err)
	}

	for _, pipelineConfig := range pipelineConfigs {
		res.ConfigDetails = append(res.ConfigDetails, pipelineConfig.Parse2ProtoPipelineConfigDetail(true))
	}
	return nil
}

func ApplyPipelineConfigToAgentGroup(req *proto.ApplyConfigToAgentGroupRequest, res *proto.ApplyConfigToAgentGroupResponse) error {
	groupName := req.GroupName
	if groupName == "" {
		return common.ValidateErrorWithMsg("required fields groupName could not be null")
	}
	configName := req.ConfigName
	if configName == "" {
		return common.ValidateErrorWithMsg("required fields configName could not be null")
	}

	var err error
	err = repository.CreatePipelineConfigForAgentGroup(groupName, configName)
	if err != nil {
		return common.SystemError(err)
	}

	agents, err := repository.ListAgentsByGroupName(groupName)
	if err != nil {
		return common.SystemError(err)
	}

	for _, agent := range agents {
		err := repository.CreatePipelineConfigForAgent(agent.InstanceId, configName)
		if err != nil {
			return err
		}
	}
	return nil
}

func RemovePipelineConfigFromAgentGroup(req *proto.RemoveConfigFromAgentGroupRequest, res *proto.RemoveConfigFromAgentGroupResponse) error {
	groupName := req.GroupName
	if groupName == "" {
		return common.ValidateErrorWithMsg("required fields groupName could not be null")
	}
	configName := req.ConfigName
	if configName == "" {
		return common.ValidateErrorWithMsg("required fields configName could not be null")
	}

	err := repository.DeletePipelineConfigForAgentGroup(groupName, configName)
	if err != nil {
		return common.SystemError(err)
	}

	agents, err := repository.ListAgentsByGroupName(groupName)
	if err != nil {
		return common.SystemError(err)
	}

	agentInstanceIds := make([]string, 0)
	for _, agent := range agents {
		agentInstanceIds = append(agentInstanceIds, agent.InstanceId)
	}

	return repository.DeletePipelineConfigForAgentInGroup(agentInstanceIds, configName)
}

func GetAppliedPipelineConfigsForAgentGroup(req *proto.GetAppliedConfigsForAgentGroupRequest, res *proto.GetAppliedConfigsForAgentGroupResponse) error {
	groupName := req.GroupName
	if groupName == "" {
		return common.ValidateErrorWithMsg("required fields groupName could not be null")
	}

	agentGroupDetail, err := repository.GetAgentGroupDetail(groupName, true, false)
	if err != nil {
		return common.SystemError(err)
	}
	configNames := make([]string, 0)
	for _, config := range agentGroupDetail.PipelineConfigs {
		configNames = append(configNames, config.Name)
	}

	res.ConfigNames = configNames
	return nil
}