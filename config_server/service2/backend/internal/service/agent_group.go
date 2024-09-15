package service

import (
	"config-server2/internal/common"
	"config-server2/internal/entity"
	"config-server2/internal/protov2"
	"config-server2/internal/repository"
	"config-server2/internal/utils"
)

// AppliedOrRemoveConfigForAgentGroup 定期检查在group中的agent与config的关系是否符合group与config的关系
func AppliedOrRemoveConfigForAgentGroup(timeLimit int64) {
	utils.TimedTask(timeLimit, func(int64) {
		agentGroupDetails, err := repository.GetAllAgentGroupDetail(true, true, true)
		if err != nil {
			panic(err)
		}
		//修改，先查出来全部的，如果全部里面有group映射的东西，那么我就留下来，否则直接删掉
		agentPipelineConfigMapList := repository.ListAgentPipelineConfig()
		agentInstanceConfigMapList := repository.ListAgentInstanceConfig()
		for _, agentGroupDetail := range agentGroupDetails {
			agents := agentGroupDetail.Agents
			pipelineConfigs := agentGroupDetail.PipelineConfigs
			instanceConfigs := agentGroupDetail.InstanceConfigs
			for _, agent := range agents {
				for _, pipelineConfig := range pipelineConfigs {
					a := entity.AgentPipelineConfig{
						AgentInstanceId:    agent.InstanceId,
						PipelineConfigName: pipelineConfig.Name,
					}
					//得两个list求差集
					if !utils.ContainElement(agentPipelineConfigMapList, a, entity.AgentPipelineConfig.Equals) {
						agentPipelineConfigMapList = append(agentPipelineConfigMapList, a)
					}
				}
			}

			for _, agent := range agents {
				for _, instanceConfig := range instanceConfigs {
					a := entity.AgentInstanceConfig{
						AgentInstanceId:    agent.InstanceId,
						InstanceConfigName: instanceConfig.Name,
					}
					if !utils.ContainElement(agentInstanceConfigMapList, a, entity.AgentInstanceConfig.Equals) {
						agentInstanceConfigMapList = append(agentInstanceConfigMapList, a)
					}
				}
			}
		}

		repository.DeleteAllPipelineConfigAndAgent()
		repository.DeleteAllInstanceConfigAndAgent()
		if agentPipelineConfigMapList != nil && len(agentPipelineConfigMapList) != 0 {
			repository.AddPipelineConfigAndAgent(agentPipelineConfigMapList)
		}
		if agentInstanceConfigMapList != nil && len(agentInstanceConfigMapList) != 0 {
			repository.AddInstanceConfigAndAgent(agentInstanceConfigMapList)
		}
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
