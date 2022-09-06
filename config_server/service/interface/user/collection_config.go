// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package user

import (
	"fmt"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/manager"
	proto "github.com/alibaba/ilogtail/config_server/service/proto"
	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"
)

func CreateConfig(c *gin.Context) {
	req := proto.CreateConfigRequest{}
	res := &proto.CreateConfigResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.ConfigDetail.ConfigName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "ConfigName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	if req.ConfigDetail.Content == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "Content")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	exist, err := manager.ConfigManager().CreateConfig(req.ConfigDetail)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else if exist {
		res.Code = common.ConfigAlreadyExist.Code
		res.Message = fmt.Sprintf("Config %s already exists.", req.ConfigDetail.ConfigName)
		c.ProtoBuf(common.ConfigAlreadyExist.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Add config success"
		c.ProtoBuf(common.Accept.Status, res)
	}
}

func UpdateConfig(c *gin.Context) {
	req := proto.UpdateConfigRequest{}
	res := &proto.UpdateConfigResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.ConfigDetail.ConfigName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "ConfigName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	if req.ConfigDetail.Content == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "Content")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	exist, err := manager.ConfigManager().UpdateConfig(req.ConfigDetail)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else if !exist {
		res.Code = common.ConfigNotExist.Code
		res.Message = fmt.Sprintf("Config %s doesn't exist.", req.ConfigDetail.ConfigName)
		c.ProtoBuf(common.ConfigNotExist.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Update config success"
		c.ProtoBuf(common.Accept.Status, res)
	}
}

func DeleteConfig(c *gin.Context) {
	req := proto.DeleteConfigRequest{}
	res := &proto.DeleteConfigResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.ConfigName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "ConfigName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	exist, err := manager.ConfigManager().DeleteConfig(req.ConfigName)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else if !exist {
		res.Code = common.ConfigNotExist.Code
		res.Message = fmt.Sprintf("Config %s doesn't exist.", req.ConfigName)
		c.ProtoBuf(common.ConfigNotExist.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Delete config success"
		c.ProtoBuf(common.Accept.Status, res)
	}
}

func GetConfig(c *gin.Context) {
	req := proto.GetConfigRequest{}
	res := &proto.GetConfigResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.ConfigName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "ConfigName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	config, err := manager.ConfigManager().GetConfig(req.ConfigName)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else if config == nil {
		res.Code = common.ConfigNotExist.Code
		res.Message = fmt.Sprintf("Config %s doesn't exist.", req.ConfigName)
		c.ProtoBuf(common.ConfigNotExist.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Get config success"
		res.ConfigDetail = config.ToProto()
		c.ProtoBuf(common.Accept.Status, res)
	}
}

func ListAllConfigs(c *gin.Context) {
	req := proto.ListConfigsRequest{}
	res := &proto.ListConfigsResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	configList, err := manager.ConfigManager().ListAllConfigs()

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Get config list success"
		res.ConfigDetails = make([]*proto.Config, 0)
		for _, v := range configList {
			res.ConfigDetails = append(res.ConfigDetails, v.ToProto())
		}
		c.ProtoBuf(common.Accept.Status, res)
	}
}

func GetAppliedAgentGroups(c *gin.Context) {
	req := proto.GetAppliedAgentGroupsRequest{}
	res := &proto.GetAppliedAgentGroupsResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.ConfigName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "ConfigName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	agentGroupList, configExist, err := manager.ConfigManager().GetAppliedAgentGroups(req.ConfigName)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else if !configExist {
		res.Code = common.ConfigNotExist.Code
		res.Message = fmt.Sprintf("Config %s doesn't exist.", req.ConfigName)
		c.ProtoBuf(common.ConfigNotExist.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Get group list success"
		res.AgentGroupNames = agentGroupList
		c.ProtoBuf(common.Accept.Status, res)
	}
}
