package handler

import (
	"config-server2/internal/common"
	proto "config-server2/internal/common/protov2"
	"config-server2/internal/service"
	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"
)

func CreateInstanceConfig(c *gin.Context) {
	request := &proto.CreateConfigRequest{}
	response := &proto.CreateConfigResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.CreateInstanceConfig(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func UpdateInstanceConfig(c *gin.Context) {
	request := &proto.UpdateConfigRequest{}
	response := &proto.UpdateConfigResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.UpdateInstanceConfig(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func DeleteInstanceConfig(c *gin.Context) {
	request := &proto.DeleteConfigRequest{}
	response := &proto.DeleteConfigResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.DeleteInstanceConfig(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func GetInstanceConfig(c *gin.Context) {
	request := &proto.GetConfigRequest{}
	response := &proto.GetConfigResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.GetInstanceConfig(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func ListInstanceConfigs(c *gin.Context) {
	request := &proto.ListConfigsRequest{}
	response := &proto.ListConfigsResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.ListInstanceConfigs(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func ApplyInstanceConfigToAgentGroup(c *gin.Context) {
	request := &proto.ApplyConfigToAgentGroupRequest{}
	response := &proto.ApplyConfigToAgentGroupResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.ApplyInstanceConfigToAgentGroup(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func RemoveInstanceConfigFromAgentGroup(c *gin.Context) {
	request := &proto.RemoveConfigFromAgentGroupRequest{}
	response := &proto.RemoveConfigFromAgentGroupResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.RemoveInstanceConfigFromAgentGroup(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func GetAppliedInstanceConfigsForAgentGroup(c *gin.Context) {
	request := &proto.GetAppliedConfigsForAgentGroupRequest{}
	response := &proto.GetAppliedConfigsForAgentGroupResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.GetAppliedInstanceConfigsForAgentGroup(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}
