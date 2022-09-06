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

func CreateAgentGroup(c *gin.Context) {
	req := proto.CreateAgentGroupRequest{}
	res := &proto.CreateAgentGroupResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.AgentGroup.GroupName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Agent group need parameter %s.", "GroupName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	exist, err := manager.ConfigManager().CreateAgentGroup(req.AgentGroup)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else if exist {
		res.Code = common.AgentGroupAlreadyExist.Code
		res.Message = fmt.Sprintf("Agent group %s already exists.", req.AgentGroup.GroupName)
		c.ProtoBuf(common.AgentGroupAlreadyExist.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Add agent group success"
		c.ProtoBuf(common.Accept.Status, res)
	}
}

func UpdateAgentGroup(c *gin.Context) {
	req := proto.UpdateAgentGroupRequest{}
	res := &proto.UpdateAgentGroupResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.AgentGroup.GroupName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Agent group need parameter %s.", "groupName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	exist, err := manager.ConfigManager().UpdateAgentGroup(req.AgentGroup)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else if !exist {
		res.Code = common.AgentGroupNotExist.Code
		res.Message = fmt.Sprintf("Agent group %s doesn't exist.", req.AgentGroup.GroupName)
		c.ProtoBuf(common.AgentGroupNotExist.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Update agent group success"
		c.ProtoBuf(common.Accept.Status, res)
	}
}

func DeleteAgentGroup(c *gin.Context) {
	req := proto.DeleteAgentGroupRequest{}
	res := &proto.DeleteAgentGroupResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.GroupName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Agent group need parameter %s.", "groupName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	exist, err := manager.ConfigManager().DeleteAgentGroup(req.GroupName)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else if !exist {
		res.Code = common.AgentGroupNotExist.Code
		res.Message = fmt.Sprintf("Agent group %s doesn't exist.", req.GroupName)
		c.ProtoBuf(common.AgentGroupNotExist.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Delete agent group success"
		c.ProtoBuf(common.Accept.Status, res)
	}
}

func GetAgentGroup(c *gin.Context) {
	req := proto.GetAgentGroupRequest{}
	res := &proto.GetAgentGroupResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.GroupName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Agent group need parameter %s.", "groupName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	agentGroup, err := manager.ConfigManager().GetAgentGroup(req.GroupName)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else if agentGroup == nil {
		res.Code = common.AgentGroupNotExist.Code
		res.Message = fmt.Sprintf("Agent group %s doesn't exist.", req.GroupName)
		c.ProtoBuf(common.AgentGroupNotExist.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Get agent group success"
		res.AgentGroup = agentGroup.ToProto()
		c.ProtoBuf(common.Accept.Status, res)
	}
}

func ListAgentGroups(c *gin.Context) {
	req := proto.ListAgentGroupsRequest{}
	res := &proto.ListAgentGroupsResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	agentGroupList, err := manager.ConfigManager().GetAllAgentGroup()

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Get agent group list success"
		res.AgentGroups = []*proto.AgentGroup{}
		for _, v := range agentGroupList {
			res.AgentGroups = append(res.AgentGroups, v.ToProto())
		}
		c.ProtoBuf(common.Accept.Status, res)
	}
}

// only default
func ListAgents(c *gin.Context) {
	req := proto.ListAgentsRequest{}
	res := &proto.ListAgentsResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.GroupName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "groupName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	agentList, err := manager.ConfigManager().GetAgentList(req.GroupName)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else if agentList == nil {
		res.Code = common.AgentGroupNotExist.Code
		res.Message = fmt.Sprintf("Agent group %s doesn't exist.", req.GroupName)
		c.ProtoBuf(common.AgentGroupNotExist.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Get agent list success"
		res.Agents = []*proto.Agent{}
		for _, v := range agentList {
			res.Agents = append(res.Agents, v.ToProto())
		}
		c.ProtoBuf(common.Accept.Status, res)
	}
}

func ApplyConfigToAgentGroup(c *gin.Context) {
	req := proto.ApplyConfigToAgentGroupRequest{}
	res := &proto.ApplyConfigToAgentGroupResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.GroupName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "groupName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	if req.ConfigName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "configName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	groupExist, configExist, hasConfig, err := manager.ConfigManager().ApplyConfigToAgentGroup(req.GroupName, req.ConfigName)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else if !groupExist {
		res.Code = common.AgentGroupNotExist.Code
		res.Message = fmt.Sprintf("Agent group %s doesn't exist.", req.GroupName)
		c.ProtoBuf(common.AgentGroupNotExist.Status, res)
	} else if !configExist {
		res.Code = common.ConfigNotExist.Code
		res.Message = fmt.Sprintf("Config %s doesn't exist.", req.ConfigName)
		c.ProtoBuf(common.ConfigNotExist.Status, res)
	} else if hasConfig {
		res.Code = common.ConfigAlreadyExist.Code
		res.Message = fmt.Sprintf("Agent group %s already has config %s.", req.GroupName, req.ConfigName)
		c.ProtoBuf(common.ConfigAlreadyExist.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Add config to agent group success"
		c.ProtoBuf(common.Accept.Status, res)
	}
}

func RemoveConfigFromAgentGroup(c *gin.Context) {
	req := proto.RemoveConfigFromAgentGroupRequest{}
	res := &proto.RemoveConfigFromAgentGroupResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.GroupName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "groupName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	if req.ConfigName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "configName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	groupExist, configExist, hasConfig, err := manager.ConfigManager().RemoveConfigFromAgentGroup(req.GroupName, req.ConfigName)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else if !groupExist {
		res.Code = common.AgentGroupNotExist.Code
		res.Message = fmt.Sprintf("Agent group %s doesn't exist.", req.GroupName)
		c.ProtoBuf(common.AgentGroupNotExist.Status, res)
	} else if !configExist {
		res.Code = common.ConfigNotExist.Code
		res.Message = fmt.Sprintf("Config %s doesn't exist.", req.ConfigName)
		c.ProtoBuf(common.ConfigNotExist.Status, res)
	} else if !hasConfig {
		res.Code = common.ConfigNotExist.Code
		res.Message = fmt.Sprintf("Agent group %s doesn't have config %s.", req.GroupName, req.ConfigName)
		c.ProtoBuf(common.ConfigNotExist.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Remove config from agent group success"
		c.ProtoBuf(common.Accept.Status, res)
	}
}

func GetAppliedConfigsForAgentGroup(c *gin.Context) {
	req := proto.GetAppliedConfigsForAgentGroupRequest{}
	res := &proto.GetAppliedConfigsForAgentGroupResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.GroupName == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "GroupName")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	configList, groupExist, err := manager.ConfigManager().GetAppliedConfigsForAgentGroup(req.GroupName)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else if !groupExist {
		res.Code = common.AgentGroupNotExist.Code
		res.Message = fmt.Sprintf("Agent group %s doesn't exist.", req.GroupName)
		c.ProtoBuf(common.AgentGroupNotExist.Status, res)
	} else if configList == nil {
		res.Code = common.ConfigNotExist.Code
		res.Message = fmt.Sprintf("Can't find some configs in agent group %s.", req.GroupName)
		c.ProtoBuf(common.ConfigNotExist.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Get agent group's applied configs success"
		res.ConfigNames = configList
		c.ProtoBuf(common.Accept.Status, res)
	}
}
