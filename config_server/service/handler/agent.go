package handler

import (
	"config-server/config"
	"config-server/service"
)

func CheckAgentExist() {
	go service.CheckAgentExist(config.ServerConfigInstance.TimeLimit)
}

var (
	HeartBeat           = ProtobufHandler(service.HeartBeat)
	FetchPipelineConfig = ProtobufHandler(service.FetchPipelineConfigDetail)
	FetchInstanceConfig = ProtobufHandler(service.FetchInstanceConfigDetail)
	ListAgentsInGroup   = ProtobufHandler(service.ListAgentsInGroup)
	//GetPipelineConfigStatusList = ProtobufHandler(service.GetPipelineConfigStatusList)
	//GetInstanceConfigStatusList = ProtobufHandler(service.GetInstanceConfigStatusList)
)
