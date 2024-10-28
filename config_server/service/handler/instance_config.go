package handler

import (
	"config-server/service"
)

var (
	CreateInstanceConfig                    = ProtobufHandler(service.CreateInstanceConfig)
	UpdateInstanceConfig                    = ProtobufHandler(service.UpdateInstanceConfig)
	DeleteInstanceConfig                    = ProtobufHandler(service.DeleteInstanceConfig)
	GetInstanceConfig                       = ProtobufHandler(service.GetInstanceConfig)
	ListInstanceConfigs                     = ProtobufHandler(service.ListInstanceConfigs)
	ApplyInstanceConfigToAgentGroup         = ProtobufHandler(service.ApplyInstanceConfigToAgentGroup)
	RemoveInstanceConfigFromAgentGroup      = ProtobufHandler(service.RemoveInstanceConfigFromAgentGroup)
	GetAppliedAgentGroupsWithInstanceConfig = ProtobufHandler(service.GetAppliedAgentGroupsForInstanceConfigName)
	GetInstanceConfigStatusList             = ProtobufHandler(service.GetInstanceConfigStatusList)
)
