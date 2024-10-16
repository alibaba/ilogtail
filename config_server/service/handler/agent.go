package handler

import (
	"config-server/common"
	"config-server/config"
	"config-server/protov2"
	"config-server/service"
	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"
)

func CheckAgentExist() {
	go service.CheckAgentExist(config.ServerConfigInstance.TimeLimit)
}

func HeartBeat(c *gin.Context) {
	request := &protov2.HeartbeatRequest{}
	response := &protov2.HeartbeatResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	err = c.ShouldBindBodyWith(request, binding.ProtoBuf)
	response.RequestId = request.RequestId
	if response.RequestId == nil {
		err = common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
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
	request := &protov2.FetchConfigRequest{}
	response := &protov2.FetchConfigResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	err = c.ShouldBindBodyWith(request, binding.ProtoBuf)
	response.RequestId = request.RequestId
	if response.RequestId == nil {
		err = common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
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
	request := &protov2.FetchConfigRequest{}
	response := &protov2.FetchConfigResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	err = c.ShouldBindBodyWith(request, binding.ProtoBuf)
	response.RequestId = request.RequestId
	if response.RequestId == nil {
		err = common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
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
	request := &protov2.ListAgentsRequest{}
	response := &protov2.ListAgentsResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	err = c.ShouldBindBodyWith(request, binding.ProtoBuf)
	response.RequestId = request.RequestId
	if response.RequestId == nil {
		err = common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
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

func GetPipelineConfigStatusList(c *gin.Context) {
	request := &protov2.GetConfigStatusListRequest{}
	response := &protov2.GetConfigStatusListResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	err = c.ShouldBindBodyWith(request, binding.ProtoBuf)
	response.RequestId = request.RequestId
	if response.RequestId == nil {
		err = common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
	if err != nil {
		err = common.SystemError(err)
		return
	}
	err = service.GetPipelineConfigStatusList(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func GetInstanceConfigStatusList(c *gin.Context) {
	request := &protov2.GetConfigStatusListRequest{}
	response := &protov2.GetConfigStatusListResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	err = c.ShouldBindBodyWith(request, binding.ProtoBuf)
	response.RequestId = request.RequestId
	if response.RequestId == nil {
		err = common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
	if err != nil {
		err = common.SystemError(err)
		return
	}
	err = service.GetInstanceConfigStatusList(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}
