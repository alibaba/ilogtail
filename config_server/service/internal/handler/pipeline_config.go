package handler

import (
	"config-server/internal/common"
	proto "config-server/internal/protov2"
	"config-server/internal/service"
	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"
)

func CreatePipelineConfig(c *gin.Context) {
	request := &proto.CreateConfigRequest{}
	response := &proto.CreateConfigResponse{}
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
	err = service.CreatePipelineConfig(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func UpdatePipelineConfig(c *gin.Context) {
	request := &proto.UpdateConfigRequest{}
	response := &proto.UpdateConfigResponse{}
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
	err = service.UpdatePipelineConfig(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func DeletePipelineConfig(c *gin.Context) {
	request := &proto.DeleteConfigRequest{}
	response := &proto.DeleteConfigResponse{}
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
	err = service.DeletePipelineConfig(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func GetPipelineConfig(c *gin.Context) {
	request := &proto.GetConfigRequest{}
	response := &proto.GetConfigResponse{}
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
	err = service.GetPipelineConfig(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func ListPipelineConfigs(c *gin.Context) {
	request := &proto.ListConfigsRequest{}
	response := &proto.ListConfigsResponse{}
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
	err = service.ListPipelineConfigs(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

// 对于之前应用到组的配置，若后续有agent加入到组里面，是否需要更新

func ApplyPipelineConfigToAgentGroup(c *gin.Context) {
	request := &proto.ApplyConfigToAgentGroupRequest{}
	response := &proto.ApplyConfigToAgentGroupResponse{}
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
	err = service.ApplyPipelineConfigToAgentGroup(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func RemovePipelineConfigFromAgentGroup(c *gin.Context) {
	request := &proto.RemoveConfigFromAgentGroupRequest{}
	response := &proto.RemoveConfigFromAgentGroupResponse{}
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
	err = service.RemovePipelineConfigFromAgentGroup(request, response)
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
	response.RequestId = request.RequestId
	if response.RequestId == nil {
		err = common.ValidateErrorWithMsg("required fields requestId could not be null")
	}
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
