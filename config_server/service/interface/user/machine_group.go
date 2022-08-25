package user

import (
	"fmt"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/manager"
	"github.com/gin-gonic/gin"
)

func CreateMachineGroup(c *gin.Context) {
	groupName := c.PostForm("groupName")
	groupTag := c.PostForm("groupTag")
	description := c.PostForm("description")

	if groupName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	exist, err := manager.ConfigManager().CreateMachineGroup(groupName, groupTag, description)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if exist {
		c.JSON(common.ErrorResponse(common.MachineGroupAlreadyExist, fmt.Sprintf("Machine group %s already exists.", groupName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Add machine group success", nil))
	}
}

func UpdateMachineGroup(c *gin.Context) {
	groupName := c.PostForm("groupName")
	groupTag := c.PostForm("groupTag")
	description := c.PostForm("description")

	if groupName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	exist, err := manager.ConfigManager().UpdateMachineGroup(groupName, groupTag, description)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if !exist {
		c.JSON(common.ErrorResponse(common.MachineGroupNotExist, fmt.Sprintf("Machine group %s doesn't exist.", groupName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Update machine group success", nil))
	}
}

func DeleteMachineGroup(c *gin.Context) {
	groupName := c.Query("groupName")
	if groupName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	exist, err := manager.ConfigManager().DeleteMachineGroup(groupName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if !exist {
		c.JSON(common.ErrorResponse(common.MachineGroupNotExist, fmt.Sprintf("Machine group %s doesn't exist.", groupName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Delete machine group success", nil))
	}
}

func GetMachineGroup(c *gin.Context) {
	groupName := c.Query("groupName")
	if groupName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	machineGroup, err := manager.ConfigManager().GetMachineGroup(groupName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if machineGroup == nil {
		c.JSON(common.ErrorResponse(common.MachineGroupNotExist, fmt.Sprintf("Machine group %s doesn't exist.", groupName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Get machine group success", gin.H{"machineGroup": machineGroup}))
	}
}

func ListMachineGroups(c *gin.Context) {
	machineGroupList, err := manager.ConfigManager().GetAllMachineGroup()

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Get machine group list success", gin.H{"machineGroupList": machineGroupList}))
	}
}

// only default
func ListMachines(c *gin.Context) {
	groupName := c.Query("groupName")
	if groupName == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	machineList, err := manager.ConfigManager().GetMachineList(groupName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if machineList == nil {
		c.JSON(common.ErrorResponse(common.MachineGroupNotExist, fmt.Sprintf("Machine group %s doesn't exist.", groupName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Get machine list success", gin.H{"machineList": machineList}))
	}
}

func ApplyConfigToMachineGroup(c *gin.Context) {
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

	groupExist, configExist, hasConfig, err := manager.ConfigManager().ApplyConfigToMachineGroup(groupName, configName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if !groupExist {
		c.JSON(common.ErrorResponse(common.MachineGroupNotExist, fmt.Sprintf("Machine group %s doesn't exist.", groupName)))
	} else if !configExist {
		c.JSON(common.ErrorResponse(common.ConfigNotExist, fmt.Sprintf("Config %s doesn't exist.", configName)))
	} else if hasConfig {
		c.JSON(common.ErrorResponse(common.ConfigAlreadyExist, fmt.Sprintf("Machine group %s already has config %s.", groupName, configName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Add config to machine group success", nil))
	}
}

func RemoveConfigFromMachineGroup(c *gin.Context) {
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

	groupExist, configExist, hasConfig, err := manager.ConfigManager().RemoveConfigFromMachineGroup(groupName, configName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if !groupExist {
		c.JSON(common.ErrorResponse(common.MachineGroupNotExist, fmt.Sprintf("Machine group %s doesn't exist.", groupName)))
	} else if !configExist {
		c.JSON(common.ErrorResponse(common.ConfigNotExist, fmt.Sprintf("Config %s doesn't exist.", configName)))
	} else if !hasConfig {
		c.JSON(common.ErrorResponse(common.ConfigNotExist, fmt.Sprintf("Machine group %s doesn't have config %s.", groupName, configName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Remove config from machine group success", nil))
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
		c.JSON(common.ErrorResponse(common.MachineGroupNotExist, fmt.Sprintf("Machine group %s doesn't exist.", groupName)))
	} else if configList == nil {
		c.JSON(common.ErrorResponse(common.ConfigNotExist, fmt.Sprintf("Can't find some configs in machine group %s.", groupName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Get machine group's applied configs success", gin.H{"configList": configList}))
	}
}
