package manager

import (
	"config-server/entity"
	proto "config-server/protov2"
)

func AddDefaultAgentGroup(agentTags []*proto.AgentGroupTag) []*proto.AgentGroupTag {
	if agentTags == nil || len(agentTags) == 0 {
		agentTags = append(agentTags, &proto.AgentGroupTag{
			Name:  entity.AgentGroupDefaultValue,
			Value: entity.AgentGroupDefaultValue,
		})
	}
	return agentTags
}

//func AppliedOrRemovePipelineConfigForAgentGroup(agentGroupDetails []*entity.AgentGroup) {
//	agentPipelineConfigList := repository.ListAgentPipelineConfig()
//	processAgentPipelineConfigMapList := make([]*entity.AgentPipelineConfig, 0)
//	for _, agentGroupDetail := range agentGroupDetails {
//		agents := agentGroupDetail.Agents
//
//		pipelineConfigs := agentGroupDetail.PipelineConfigs
//		for _, agent := range agents {
//			for _, pipelineConfig := range pipelineConfigs {
//				a := &entity.AgentPipelineConfig{
//					AgentInstanceId:    agent.InstanceId,
//					PipelineConfigName: pipelineConfig.Name,
//				}
//				//以agentGroup中 group与agent的关系为主
//				if !utils.ContainElement(processAgentPipelineConfigMapList, a, (*entity.AgentPipelineConfig).Equals) {
//					processAgentPipelineConfigMapList = append(processAgentPipelineConfigMapList, a)
//				}
//			}
//		}
//		// 如果agent与config的关系 存在于 agentGroup与config的关系中，则以前者为主
//		for _, agentPipelineConfig := range agentPipelineConfigList {
//			utils.ReplaceElement(processAgentPipelineConfigMapList, agentPipelineConfig, (*entity.AgentPipelineConfig).Equals)
//		}
//
//	}
//	repository.DeleteAllPipelineConfigAndAgent()
//	if processAgentPipelineConfigMapList != nil && len(processAgentPipelineConfigMapList) != 0 {
//		err := CreateOrUpdateAgentPipelineConfigs(processAgentPipelineConfigMapList)
//		if err != nil {
//			panic(err)
//		}
//	}
//}
//
//func AppliedOrRemoveInstanceConfigForAgentGroup(agentGroupDetails []*entity.AgentGroup) {
//	agentInstanceConfigList := repository.ListAgentInstanceConfig()
//	processAgentInstanceConfigMapList := make([]*entity.AgentInstanceConfig, 0)
//	for _, agentGroupDetail := range agentGroupDetails {
//		agents := agentGroupDetail.Agents
//
//		instanceConfigs := agentGroupDetail.InstanceConfigs
//		for _, agent := range agents {
//			for _, instanceConfig := range instanceConfigs {
//				a := &entity.AgentInstanceConfig{
//					AgentInstanceId:    agent.InstanceId,
//					InstanceConfigName: instanceConfig.Name,
//				}
//				//以agentGroup中 group与agent的关系为主
//				if !utils.ContainElement(processAgentInstanceConfigMapList, a, (*entity.AgentInstanceConfig).Equals) {
//					processAgentInstanceConfigMapList = append(processAgentInstanceConfigMapList, a)
//				}
//			}
//		}
//		// 如果agent与config的关系 存在于 agentGroup与config的关系中，则以前者为主
//		for _, agentInstanceConfig := range agentInstanceConfigList {
//			utils.ReplaceElement(processAgentInstanceConfigMapList, agentInstanceConfig, (*entity.AgentInstanceConfig).Equals)
//		}
//
//	}
//
//	repository.DeleteAllInstanceConfigAndAgent()
//	if processAgentInstanceConfigMapList != nil && len(processAgentInstanceConfigMapList) != 0 {
//		err := CreateOrUpdateAgentInstanceConfigs(processAgentInstanceConfigMapList)
//		if err != nil {
//			panic(err)
//		}
//	}
//
//}
