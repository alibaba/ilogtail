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
	"github.com/gin-gonic/gin"
)

func CreateAgentGroup(c *gin.Context) {
	groupName := c.PostForm("groupName")
	groupTag := c.PostForm("groupTag")
	description := c.PostForm("description")

	if groupName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	exist, err := manager.ConfigManager().CreateAgentGroup(groupName, groupTag, description)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if exist {
		c.JSON(common.ErrorResponse(common.AgentGroupAlreadyExist, fmt.Sprintf("Agent group %s already exists.", groupName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Add agent group success", nil))
	}
}

func UpdateAgentGroup(c *gin.Context) {
	groupName := c.PostForm("groupName")
	groupTag := c.PostForm("groupTag")
	description := c.PostForm("description")

	if groupName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	exist, err := manager.ConfigManager().UpdateAgentGroup(groupName, groupTag, description)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if !exist {
		c.JSON(common.ErrorResponse(common.AgentGroupNotExist, fmt.Sprintf("Agent group %s doesn't exist.", groupName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Update agent group success", nil))
	}
}

func DeleteAgentGroup(c *gin.Context) {
	groupName := c.Query("groupName")
	if groupName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	exist, err := manager.ConfigManager().DeleteAgentGroup(groupName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if !exist {
		c.JSON(common.ErrorResponse(common.AgentGroupNotExist, fmt.Sprintf("Agent group %s doesn't exist.", groupName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Delete agent group success", nil))
	}
}

func GetAgentGroup(c *gin.Context) {
	groupName := c.Query("groupName")
	if groupName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	agentGroup, err := manager.ConfigManager().GetAgentGroup(groupName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if agentGroup == nil {
		c.JSON(common.ErrorResponse(common.AgentGroupNotExist, fmt.Sprintf("Agent group %s doesn't exist.", groupName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Get agent group success", gin.H{"agentGroup": agentGroup}))
	}
}

func ListAgentGroups(c *gin.Context) {
	agentGroupList, err := manager.ConfigManager().GetAllAgentGroup()

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Get agent group list success", gin.H{"agentGroupList": agentGroupList}))
	}
}

// only default
func ListAgents(c *gin.Context) {
	groupName := c.Query("groupName")
	if groupName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	agentList, err := manager.ConfigManager().GetAgentList(groupName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if agentList == nil {
		c.JSON(common.ErrorResponse(common.AgentGroupNotExist, fmt.Sprintf("Agent group %s doesn't exist.", groupName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Get agent list success", gin.H{"agentList": agentList}))
	}
}

func ApplyConfigToAgentGroup(c *gin.Context) {
	groupName := c.PostForm("groupName")
	configName := c.PostForm("configName")
	if groupName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}
	if configName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configName")))
		return
	}

	groupExist, configExist, hasConfig, err := manager.ConfigManager().ApplyConfigToAgentGroup(groupName, configName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if !groupExist {
		c.JSON(common.ErrorResponse(common.AgentGroupNotExist, fmt.Sprintf("Agent group %s doesn't exist.", groupName)))
	} else if !configExist {
		c.JSON(common.ErrorResponse(common.ConfigNotExist, fmt.Sprintf("Config %s doesn't exist.", configName)))
	} else if hasConfig {
		c.JSON(common.ErrorResponse(common.ConfigAlreadyExist, fmt.Sprintf("Agent group %s already has config %s.", groupName, configName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Add config to agent group success", nil))
	}
}

func RemoveConfigFromAgentGroup(c *gin.Context) {
	groupName := c.PostForm("groupName")
	configName := c.PostForm("configName")
	if groupName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}
	if configName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configName")))
		return
	}

	groupExist, configExist, hasConfig, err := manager.ConfigManager().RemoveConfigFromAgentGroup(groupName, configName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if !groupExist {
		c.JSON(common.ErrorResponse(common.AgentGroupNotExist, fmt.Sprintf("Agent group %s doesn't exist.", groupName)))
	} else if !configExist {
		c.JSON(common.ErrorResponse(common.ConfigNotExist, fmt.Sprintf("Config %s doesn't exist.", configName)))
	} else if !hasConfig {
		c.JSON(common.ErrorResponse(common.ConfigNotExist, fmt.Sprintf("Agent group %s doesn't have config %s.", groupName, configName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Remove config from agent group success", nil))
	}
}

func GetAppliedConfigs(c *gin.Context) {
	groupName := c.Query("groupName")
	if groupName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	configList, groupExist, err := manager.ConfigManager().GetAppliedConfigs(groupName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if !groupExist {
		c.JSON(common.ErrorResponse(common.AgentGroupNotExist, fmt.Sprintf("Agent group %s doesn't exist.", groupName)))
	} else if configList == nil {
		c.JSON(common.ErrorResponse(common.ConfigNotExist, fmt.Sprintf("Can't find some configs in agent group %s.", groupName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Get agent group's applied configs success", gin.H{"configList": configList}))
	}
}
