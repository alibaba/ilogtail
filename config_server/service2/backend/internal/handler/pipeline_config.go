package handler

import (
	"config-server2/internal/common"
	proto "config-server2/internal/protov2"
	"config-server2/internal/service"
	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"
)

func CreatePipelineConfig(c *gin.Context) {
	request := &proto.CreateConfigRequest{}
	response := &proto.CreateConfigResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.CreatePipelineConfig(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func UpdatePipelineConfig(c *gin.Context) {
	request := &proto.UpdateConfigRequest{}
	response := &proto.UpdateConfigResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.UpdatePipelineConfig(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func DeletePipelineConfig(c *gin.Context) {
	request := &proto.DeleteConfigRequest{}
	response := &proto.DeleteConfigResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.DeletePipelineConfig(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func GetPipelineConfig(c *gin.Context) {
	request := &proto.GetConfigRequest{}
	response := &proto.GetConfigResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.GetPipelineConfig(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func ListPipelineConfigs(c *gin.Context) {
	request := &proto.ListConfigsRequest{}
	response := &proto.ListConfigsResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.ListPipelineConfigs(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

// 对于之前应用到组的配置，若后续有agent加入到组里面，是否需要更新
func ApplyPipelineConfigToAgentGroup(c *gin.Context) {
	request := &proto.ApplyConfigToAgentGroupRequest{}
	response := &proto.ApplyConfigToAgentGroupResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}

	err = service.ApplyPipelineConfigToAgentGroup(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func RemovePipelineConfigFromAgentGroup(c *gin.Context) {
	request := &proto.RemoveConfigFromAgentGroupRequest{}
	response := &proto.RemoveConfigFromAgentGroupResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.RemovePipelineConfigFromAgentGroup(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}

func GetAppliedPipelineConfigsForAgentGroup(c *gin.Context) {
	request := &proto.GetAppliedConfigsForAgentGroupRequest{}
	response := &proto.GetAppliedConfigsForAgentGroupResponse{}
	err := c.ShouldBindBodyWith(request, binding.ProtoBuf)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	err = service.GetAppliedPipelineConfigsForAgentGroup(request, response)
	if err != nil {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.ErrorProtobufRes(c, response, err)
		return
	}
	common.SuccessProtobufRes(c, response)
}
