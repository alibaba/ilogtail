package service

import (
	"config-server/common"
	"config-server/entity"
	proto "config-server/protov2"
	"config-server/repository"
	"config-server/utils"
)

func CreatePipelineConfig(req *proto.CreateConfigRequest, res *proto.CreateConfigResponse) error {
	configDetail := req.ConfigDetail
	if utils.IsEmptyOrWhitespace(configDetail.Name) {
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}

	if configDetail.Version <= 0 {
		return common.ValidateErrorWithMsg("required field version could not less than 0")
	}

	pipelineConfig := entity.ParseProtoPipelineConfig2PipelineConfig(configDetail)
	err := repository.CreatePipelineConfig(pipelineConfig)
	return common.SystemError(err)

}

//更新配置的时候version也要更新

func UpdatePipelineConfig(req *proto.UpdateConfigRequest, res *proto.UpdateConfigResponse) error {
	configDetail := req.ConfigDetail
	if configDetail.Name == "" {
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}

	if configDetail.Version == 0 {
		return common.ValidateErrorWithMsg("required field version could not be null")
	}
	configDetail.Version += 1
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

	return nil
}

func RemovePipelineConfigFromAgentGroup(req *proto.RemoveConfigFromAgentGroupRequest, res *proto.RemoveConfigFromAgentGroupResponse) error {
	groupName := req.GroupName
	if groupName == "" {
		return common.ValidateErrorWithMsg("required fields groupName could not be null")
	}
	configName := req.ConfigName

	err := repository.DeletePipelineConfigForAgentGroup(groupName, configName)
	if err != nil {
		return common.SystemError(err)
	}
	return nil
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

func GetPipelineConfigStatusList(req *proto.GetConfigStatusListRequest, res *proto.GetConfigStatusListResponse) error {
	instanceId := req.InstanceId
	if instanceId == nil {
		return common.ValidateErrorWithMsg("required fields instanceId could not be null")
	}
	agent, err := repository.GetAgentByID(string(instanceId), "instance_id", "pipeline_config_statuses")
	if err != nil {
		return common.SystemError(err)
	}
	res.ConfigStatus = agent.PipelineConfigStatuses.Parse2ProtoConfigStatus()
	return nil
}
