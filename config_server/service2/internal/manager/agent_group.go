package manager

import "config-server2/internal/entity"

func AddDefaultAgentGroup(agentTags []*entity.AgentGroup) []*entity.AgentGroup {
	if agentTags == nil || len(agentTags) == 0 {
		agentTags = append(agentTags, &entity.AgentGroup{
			Name:  entity.AgentGroupDefaultValue,
			Value: entity.AgentGroupDefaultValue,
		})
	}
	return agentTags
}
