package handler

import (
	"config-server2/internal/common"
	proto "config-server2/internal/protov2"
	"config-server2/internal/service"
	"github.com/gin-gonic/gin"
)

func CreateInstanceConfig(c *gin.Context) {
	request := &proto.CreateConfigRequest{}
	response := &proto.CreateConfigResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	if err != nil {
		err = common.SystemError(err)
		return
	}
	err = service.CreateInstanceConfig(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func UpdateInstanceConfig(c *gin.Context) {
	request := &proto.UpdateConfigRequest{}
	response := &proto.UpdateConfigResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	if err != nil {
		err = common.SystemError(err)
		return
	}
	err = service.UpdateInstanceConfig(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func DeleteInstanceConfig(c *gin.Context) {
	request := &proto.DeleteConfigRequest{}
	response := &proto.DeleteConfigResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	if err != nil {
		err = common.SystemError(err)
		return
	}
	err = service.DeleteInstanceConfig(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func GetInstanceConfig(c *gin.Context) {
	request := &proto.GetConfigRequest{}
	response := &proto.GetConfigResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	if err != nil {
		err = common.SystemError(err)
		return
	}
	err = service.GetInstanceConfig(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func ListInstanceConfigs(c *gin.Context) {
	request := &proto.ListConfigsRequest{}
	response := &proto.ListConfigsResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	if err != nil {
		err = common.SystemError(err)
		return
	}
	err = service.ListInstanceConfigs(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func ApplyInstanceConfigToAgentGroup(c *gin.Context) {
	request := &proto.ApplyConfigToAgentGroupRequest{}
	response := &proto.ApplyConfigToAgentGroupResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	if err != nil {
		err = common.SystemError(err)
		return
	}
	err = service.ApplyInstanceConfigToAgentGroup(request, response)
	if err != nil {
		err = common.SystemError(err)
		return
	}
}

func RemoveInstanceConfigFromAgentGroup(c *gin.Context) {
	request := &proto.RemoveConfigFromAgentGroupRequest{}
	response := &proto.RemoveConfigFromAgentGroupResponse{}
	var err error
	defer func() {
		response.CommonResponse = common.GenerateCommonResponse(err)
		common.SuccessProtobufRes(c, response)
	}()
	if err != nil {
		err = common.SystemError(err)
		return
	}
	err = service.RemoveInstanceConfigFromAgentGroup(request, response)
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
