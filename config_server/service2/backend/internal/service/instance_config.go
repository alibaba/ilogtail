package service

import (
	"config-server2/internal/common"
	"config-server2/internal/entity"
	proto "config-server2/internal/protov2"
	"config-server2/internal/repository"
	"config-server2/internal/utils"
)

func CreateInstanceConfig(req *proto.CreateConfigRequest, res *proto.CreateConfigResponse) error {
	configDetail := req.ConfigDetail
	if utils.IsEmptyOrWhitespace(configDetail.Name) {
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}

	if configDetail.Version == 0 {
		return common.ValidateErrorWithMsg("required field version could not be null")
	}
	instanceConfig := entity.ParseProtoInstanceConfig2InstanceConfig(configDetail)
	err := repository.CreateInstanceConfig(instanceConfig)
	return common.SystemError(err)

}

func UpdateInstanceConfig(req *proto.UpdateConfigRequest, res *proto.UpdateConfigResponse) error {
	configDetail := req.ConfigDetail
	if configDetail.Name == "" {
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}

	if configDetail.Version == 0 {
		return common.ValidateErrorWithMsg("required field version could not be null")
	}
	instanceConfig := entity.ParseProtoInstanceConfig2InstanceConfig(configDetail)
	err := repository.UpdateInstanceConfig(instanceConfig)
	return common.SystemError(err)
}

func DeleteInstanceConfig(req *proto.DeleteConfigRequest, res *proto.DeleteConfigResponse) error {
	configName := req.ConfigName
	if configName == "" {
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}
	err := repository.DeleteInstanceConfig(configName)
	return common.SystemError(err)
}

func GetInstanceConfig(req *proto.GetConfigRequest, res *proto.GetConfigResponse) error {
	configName := req.ConfigName
	if configName == "" {
		return common.ValidateErrorWithMsg("required field configName could not be null")
	}

	instanceConfig, err := repository.GetInstanceConfig(configName)
	if err != nil {
		return common.SystemError(err)
	}
	if instanceConfig != nil {
		res.ConfigDetail = instanceConfig.Parse2ProtoInstanceConfigDetail(true)
	}
	return common.SystemError(err)
}

func ListInstanceConfigs(req *proto.ListConfigsRequest, res *proto.ListConfigsResponse) error {
	instanceConfigs, err := repository.ListInstanceConfigs()
	if err != nil {
		return common.SystemError(err)
	}

	for _, instanceConfig := range instanceConfigs {
		res.ConfigDetails = append(res.ConfigDetails, instanceConfig.Parse2ProtoInstanceConfigDetail(true))
	}
	return nil
}

func ApplyInstanceConfigToAgentGroup(req *proto.ApplyConfigToAgentGroupRequest, res *proto.ApplyConfigToAgentGroupResponse) error {
	groupName := req.GroupName
	if groupName == "" {
		return common.ValidateErrorWithMsg("required fields groupName could not be null")
	}
	configName := req.ConfigName
	if configName == "" {
		return common.ValidateErrorWithMsg("required fields configName could not be null")
	}

	var err error
	err = repository.CreateInstanceConfigForAgentGroup(groupName, configName)
	if err != nil {
		return common.SystemError(err)
	}

	agents, err := repository.ListAgentsByGroupName(groupName)
	if err != nil {
		return common.SystemError(err)
	}

	for _, agent := range agents {
		err := repository.CreateInstanceConfigForAgent(agent.InstanceId, configName)
		if err != nil {
			return err
		}
	}
	return nil
}

func RemoveInstanceConfigFromAgentGroup(req *proto.RemoveConfigFromAgentGroupRequest, res *proto.RemoveConfigFromAgentGroupResponse) error {
	groupName := req.GroupName
	if groupName == "" {
		return common.ValidateErrorWithMsg("required fields groupName could not be null")
	}
	configName := req.ConfigName

	err := repository.DeleteInstanceConfigForAgentGroup(groupName, configName)
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

	return repository.DeleteInstanceConfigForAgentInGroup(agentInstanceIds, configName)
}

func GetAppliedInstanceConfigsForAgentGroup(req *proto.GetAppliedConfigsForAgentGroupRequest, res *proto.GetAppliedConfigsForAgentGroupResponse) error {
	groupName := req.GroupName
	if groupName == "" {
		return common.ValidateErrorWithMsg("required fields groupName could not be null")
	}

	agentGroupDetail, err := repository.GetAgentGroupDetail(groupName, false, true)
	if err != nil {
		return common.SystemError(err)
	}
	configNames := make([]string, 0)
	for _, config := range agentGroupDetail.InstanceConfigs {
		configNames = append(configNames, config.Name)
	}

	res.ConfigNames = configNames
	return nil
}