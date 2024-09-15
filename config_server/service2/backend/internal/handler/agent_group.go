package handler

import (
	"config-server2/internal/common"
	proto "config-server2/internal/protov2"
	"config-server2/internal/service"
	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"
)

func AppliedOrRemoveConfigForAgentGroup() {
	go service.AppliedOrRemoveConfigForAgentGroup(15)
}

func CreateAgentGroup(c *gin.Context) {
	request := &proto.CreateAgentGroupRequest{}
	response := &proto.CreateAgentGroupResponse{}
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
	err = service.CreateAgentGroup(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func UpdateAgentGroup(c *gin.Context) {
	request := &proto.UpdateAgentGroupRequest{}
	response := &proto.UpdateAgentGroupResponse{}
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
	err = service.UpdateAgentGroup(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func DeleteAgentGroup(c *gin.Context) {
	request := &proto.DeleteAgentGroupRequest{}
	response := &proto.DeleteAgentGroupResponse{}
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
	err = service.DeleteAgentGroup(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func GetAgentGroup(c *gin.Context) {
	request := &proto.GetAgentGroupRequest{}
	response := &proto.GetAgentGroupResponse{}
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
	err = service.GetAgentGroup(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func ListAgentGroups(c *gin.Context) {
	request := &proto.ListAgentGroupsRequest{}
	response := &proto.ListAgentGroupsResponse{}
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
	err = service.ListAgentGroups(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func GetAppliedPipelineConfigsForAgentGroup(c *gin.Context) {
	request := &proto.GetAppliedConfigsForAgentGroupRequest{}
	response := &proto.GetAppliedConfigsForAgentGroupResponse{}
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
	err = service.GetAppliedPipelineConfigsForAgentGroup(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func GetAppliedInstanceConfigsForAgentGroup(c *gin.Context) {
	request := &proto.GetAppliedConfigsForAgentGroupRequest{}
	response := &proto.GetAppliedConfigsForAgentGroupResponse{}
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
	err = service.GetAppliedInstanceConfigsForAgentGroup(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}
