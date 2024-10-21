package handler

import (
	"config-server/service"
)

var (
	CreatePipelineConfig                    = ProtobufHandler(service.CreatePipelineConfig)
	UpdatePipelineConfig                    = ProtobufHandler(service.UpdatePipelineConfig)
	DeletePipelineConfig                    = ProtobufHandler(service.DeletePipelineConfig)
	GetPipelineConfig                       = ProtobufHandler(service.GetPipelineConfig)
	ListPipelineConfigs                     = ProtobufHandler(service.ListPipelineConfigs)
	ApplyPipelineConfigToAgentGroup         = ProtobufHandler(service.ApplyPipelineConfigToAgentGroup)
	RemovePipelineConfigFromAgentGroup      = ProtobufHandler(service.RemovePipelineConfigFromAgentGroup)
	GetAppliedAgentGroupsWithPipelineConfig = ProtobufHandler(service.GetAppliedAgentGroupsForPipelineConfigName)
)
