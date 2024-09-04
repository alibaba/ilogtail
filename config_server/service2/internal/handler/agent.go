package handler

import (
	"config-server2/internal/common"
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/config"
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
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.HeartBeat(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func FetchPipelineConfig(c *gin.Context) {
	request := &proto.FetchConfigRequest{}
	response := &proto.FetchConfigResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.FetchPipelineConfigDetail(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func FetchInstanceConfig(c *gin.Context) {
	request := &proto.FetchConfigRequest{}
	response := &proto.FetchConfigResponse{}
	err := c.ShouldBindBodyWith(&request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.FetchInstanceConfigDetail(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func ListAgentsInGroup(c *gin.Context) {
	request := &proto.ListAgentsRequest{}
	response := &proto.ListAgentsResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.ListAgentsInGroup(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}
