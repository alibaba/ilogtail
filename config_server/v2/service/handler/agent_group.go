package handler

import (
	"config-server/service"
)

var (
	CreateAgentGroup                       = ProtobufHandler(service.CreateAgentGroup)
	UpdateAgentGroup                       = ProtobufHandler(service.UpdateAgentGroup)
	DeleteAgentGroup                       = ProtobufHandler(service.DeleteAgentGroup)
	GetAgentGroup                          = ProtobufHandler(service.GetAgentGroup)
	ListAgentGroups                        = ProtobufHandler(service.ListAgentGroups)
	GetAppliedPipelineConfigsForAgentGroup = ProtobufHandler(service.GetAppliedPipelineConfigsForAgentGroup)
	GetAppliedInstanceConfigsForAgentGroup = ProtobufHandler(service.GetAppliedInstanceConfigsForAgentGroup)
)
