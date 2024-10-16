package handler

import (
	"config-server/service"
)

func AppliedOrRemoveConfigForAgentGroup() {
	go service.AppliedOrRemoveConfigForAgentGroup(30)
}

var (
	CreateAgentGroup                       = ProtobufHandler(service.CreateAgentGroup)
	UpdateAgentGroup                       = ProtobufHandler(service.UpdateAgentGroup)
	DeleteAgentGroup                       = ProtobufHandler(service.DeleteAgentGroup)
	GetAgentGroup                          = ProtobufHandler(service.GetAgentGroup)
	ListAgentGroups                        = ProtobufHandler(service.ListAgentGroups)
	GetAppliedPipelineConfigsForAgentGroup = ProtobufHandler(service.GetAppliedPipelineConfigsForAgentGroup)
	GetAppliedInstanceConfigsForAgentGroup = ProtobufHandler(service.GetAppliedInstanceConfigsForAgentGroup)
)
