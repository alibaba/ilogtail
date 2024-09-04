package handler

import (
	"config-server2/internal/common"
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/service"
	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"
)

func CreateAgentGroup(c *gin.Context) {
	request := &proto.CreateAgentGroupRequest{}
	response := &proto.CreateAgentGroupResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.CreateAgentGroup(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func UpdateAgentGroup(c *gin.Context) {
	request := &proto.UpdateAgentGroupRequest{}
	response := &proto.UpdateAgentGroupResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.UpdateAgentGroup(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func DeleteAgentGroup(c *gin.Context) {
	request := &proto.DeleteAgentGroupRequest{}
	response := &proto.DeleteAgentGroupResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.DeleteAgentGroup(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func GetAgentGroup(c *gin.Context) {
	request := &proto.GetAgentGroupRequest{}
	response := &proto.GetAgentGroupResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.GetAgentGroup(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func ListAgentGroups(c *gin.Context) {
	request := &proto.ListAgentGroupsRequest{}
	response := &proto.ListAgentGroupsResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.ListAgentGroups(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func GetAppliedAgentGroupsWithPipelineConfig(c *gin.Context) {
	request := &proto.GetAppliedAgentGroupsRequest{}
	response := &proto.GetAppliedAgentGroupsResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.GetAppliedAgentGroupsForPipelineConfigName(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func GetAppliedAgentGroupsWithInstanceConfig(c *gin.Context) {
	request := &proto.GetAppliedAgentGroupsRequest{}
	response := &proto.GetAppliedAgentGroupsResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.GetAppliedAgentGroupsForInstanceConfigName(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}
