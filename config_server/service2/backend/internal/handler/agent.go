package handler

import (
	"config-server2/internal/common"
	"config-server2/internal/config"
	proto "config-server2/internal/protov2"
	"config-server2/internal/service"
	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"
)

func CheckAgentExist() {
	go service.CheckAgentExist(config.ServerConfigInstance.TimeLimit)
}

func HeartBeat(c *gin.Context) {
	request := &proto.HeartbeatRequest{}
	response := &proto.HeartbeatResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	err = c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		err = common.SystemError(err)
		return
	}
	err = service.HeartBeat(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func FetchPipelineConfig(c *gin.Context) {
	request := &proto.FetchConfigRequest{}
	response := &proto.FetchConfigResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	err = c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		err = common.SystemError(err)
		return
	}
	err = service.FetchPipelineConfigDetail(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func FetchInstanceConfig(c *gin.Context) {
	request := &proto.FetchConfigRequest{}
	response := &proto.FetchConfigResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	err = c.ShouldBindBodyWith(&request, binding.ProtoBuf)
	if err != nil {
		err = common.SystemError(err)
		return
	}
	err = service.FetchInstanceConfigDetail(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func ListAgentsInGroup(c *gin.Context) {
	request := &proto.ListAgentsRequest{}
	response := &proto.ListAgentsResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	err = c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		err = common.SystemError(err)
		return
	}
	err = service.ListAgentsInGroup(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}
