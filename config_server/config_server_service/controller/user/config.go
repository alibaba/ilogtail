package user

import (
	"fmt"

	"github.com/alibaba/ilogtail/config_server/config_server_service/common"
	configmanager "github.com/alibaba/ilogtail/config_server/config_server_service/config_manager"
	"github.com/gin-gonic/gin"
)

func CreateConfig(c *gin.Context) {
	configName := c.PostForm("configName")
	configInfo := c.PostForm("configInfo")
	description := c.PostForm("description")

	if configName == "" {
		c.JSON(400, common.Error(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configName")))
		return
	}

	if configInfo == "" {
		c.JSON(400, common.Error(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configInfo")))
		return
	}

	exist, err := configmanager.CreateConfig(configName, configInfo, description)

	if err != nil {
		c.JSON(500, common.Error(common.InternalServerError, err.Error()))
	} else if exist {
		c.JSON(400, common.Error(common.ConfigAlreadyExist, fmt.Sprintf("Config %s already exists.", configName)))
	} else {
		c.JSON(200, common.Accept("Add config success", nil))
	}
}

func UpdateConfig(c *gin.Context) {
	configName := c.PostForm("configName")
	configInfo := c.PostForm("configInfo")
	description := c.PostForm("description")

	if configName == "" {
		c.JSON(400, common.Error(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configName")))
		return
	}

	if configInfo == "" {
		c.JSON(400, common.Error(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configInfo")))
		return
	}

	exist, err := configmanager.UpdateConfig(configName, configInfo, description)

	if err != nil {
		c.JSON(500, common.Error(common.InternalServerError, err.Error()))
	} else if !exist {
		c.JSON(404, common.Error(common.ConfigNotExist, fmt.Sprintf("Config %s doesn't exist.", configName)))
	} else {
		c.JSON(200, common.Accept("Update config success", nil))
	}
}

func DeleteConfig(c *gin.Context) {
	configName := c.Query("configName")
	if configName == "" {
		c.JSON(400, common.Error(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configName")))
		return
	}

	exist, err := configmanager.DeleteConfig(configName)

	if err != nil {
		c.JSON(500, common.Error(common.InternalServerError, err.Error()))
	} else if !exist {
		c.JSON(404, common.Error(common.ConfigNotExist, fmt.Sprintf("Config %s doesn't exist.", configName)))
	} else {
		c.JSON(200, common.Accept("Delete config success", nil))
	}
}

func GetConfig(c *gin.Context) {
	configName := c.Query("configName")
	if configName == "" {
		c.JSON(400, common.Error(common.BadRequest, fmt.Sprintf("Need parameter %s.", "configName")))
		return
	}

	config, err := configmanager.GetConfig(configName)

	if err != nil {
		c.JSON(500, common.Error(common.InternalServerError, err.Error()))
	} else if config == nil {
		c.JSON(404, common.Error(common.ConfigNotExist, fmt.Sprintf("Config %s doesn't exist.", configName)))
	} else {
		c.JSON(200, common.Accept("Get config success", gin.H{"config": config}))
	}
}

func ListAllConfigs(c *gin.Context) {
	configList, err := configmanager.ListAllConfigs()

	if err != nil {
		c.JSON(500, common.Error(common.InternalServerError, err.Error()))
	} else {
		c.JSON(200, common.Accept("Get config list success", gin.H{"configList": configList}))
	}
}
