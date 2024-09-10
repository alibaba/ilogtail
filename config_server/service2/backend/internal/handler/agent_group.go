package handler

import (
	"config-server2/internal/common"
	proto "config-server2/internal/protov2"
	"config-server2/internal/service"
	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"
)

func CreateAgentGroup(c *gin.Context) {
	request := &proto.CreateAgentGroupRequest{}
	response := &proto.CreateAgentGroupResponse{}
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

func GetAppliedAgentGroupsWithPipelineConfig(c *gin.Context) {
	request := &proto.GetAppliedAgentGroupsRequest{}
	response := &proto.GetAppliedAgentGroupsResponse{}
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
	err = service.GetAppliedAgentGroupsForPipelineConfigName(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func GetAppliedAgentGroupsWithInstanceConfig(c *gin.Context) {
	request := &proto.GetAppliedAgentGroupsRequest{}
	response := &proto.GetAppliedAgentGroupsResponse{}
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
	err = service.GetAppliedAgentGroupsForInstanceConfigName(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}
