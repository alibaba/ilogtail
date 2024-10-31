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
