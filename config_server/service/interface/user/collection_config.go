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

func CreateConfig(c *gin.Context) {
	configName := c.PostForm("configName")
	configInfo := c.PostForm("configInfo")
	description := c.PostForm("description")

	if configName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configName")))
		return
	}

	if configInfo == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configInfo")))
		return
	}

	exist, err := manager.ConfigManager().CreateConfig(configName, configInfo, description)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if exist {
		c.JSON(common.ErrorResponse(common.ConfigAlreadyExist, fmt.Sprintf("Config %s already exists.", configName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Add config success", nil))
	}
}

func UpdateConfig(c *gin.Context) {
	configName := c.PostForm("configName")
	configInfo := c.PostForm("configInfo")
	description := c.PostForm("description")

	if configName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configName")))
		return
	}

	if configInfo == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configInfo")))
		return
	}

	exist, err := manager.ConfigManager().UpdateConfig(configName, configInfo, description)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if !exist {
		c.JSON(common.ErrorResponse(common.ConfigNotExist, fmt.Sprintf("Config %s doesn't exist.", configName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Update config success", nil))
	}
}

func DeleteConfig(c *gin.Context) {
	configName := c.Query("configName")
	if configName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configName")))
		return
	}

	exist, err := manager.ConfigManager().DeleteConfig(configName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if !exist {
		c.JSON(common.ErrorResponse(common.ConfigNotExist, fmt.Sprintf("Config %s doesn't exist.", configName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Delete config success", nil))
	}
}

func GetConfig(c *gin.Context) {
	configName := c.Query("configName")
	if configName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configName")))
		return
	}

	config, err := manager.ConfigManager().GetConfig(configName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if config == nil {
		c.JSON(common.ErrorResponse(common.ConfigNotExist, fmt.Sprintf("Config %s doesn't exist.", configName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Get config success", gin.H{"config": config}))
	}
}

func ListAllConfigs(c *gin.Context) {
	configList, err := manager.ConfigManager().ListAllConfigs()

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Get config list success", gin.H{"configList": configList}))
	}
}

func GetAppliedAgentGroups(c *gin.Context) {
	configName := c.Query("configName")
	if configName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configName")))
		return
	}

	agentGroupList, configExist, err := manager.ConfigManager().GetAppliedAgentGroups(configName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if !configExist {
		c.JSON(common.ErrorResponse(common.ConfigNotExist, fmt.Sprintf("Config %s doesn't exist.", configName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Get group list success", gin.H{"groupList": agentGroupList}))
	}
}
