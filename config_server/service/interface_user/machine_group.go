package user

import (
	"fmt"

	"github.com/alibaba/ilogtail/config_server/service/common"
	configmanager "github.com/alibaba/ilogtail/config_server/service/manager_config"
	"github.com/gin-gonic/gin"
)

func CreateMachineGroup(c *gin.Context) {
	groupName := c.PostForm("groupName")
	groupTag := c.PostForm("groupTag")
	description := c.PostForm("description")

	if groupName == "" {
		c.JSON(400, common.Error(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	exist, err := configmanager.CreateMachineGroup(groupName, groupTag, description)

	if err != nil {
		c.JSON(500, common.Error(common.InternalServerError, err.Error()))
	} else if exist {
		c.JSON(400, common.Error(common.MachineGroupAlreadyExist, fmt.Sprintf("Machine group %s already exists.", groupName)))
	} else {
		c.JSON(200, common.Accept("Add machine group success", nil))
	}
}

func UpdateMachineGroup(c *gin.Context) {
	groupName := c.PostForm("groupName")
	groupTag := c.PostForm("groupTag")
	description := c.PostForm("description")

	if groupName == "" {
		c.JSON(400, common.Error(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	exist, err := configmanager.UpdateMachineGroup(groupName, groupTag, description)

	if err != nil {
		c.JSON(500, common.Error(common.InternalServerError, err.Error()))
	} else if !exist {
		c.JSON(404, common.Error(common.MachineGroupNotExist, fmt.Sprintf("Machine group %s doesn't exist.", groupName)))
	} else {
		c.JSON(200, common.Accept("Update machine group success", nil))
	}
}

func DeleteMachineGroup(c *gin.Context) {
	groupName := c.Query("groupName")
	if groupName == "" {
		c.JSON(400, common.Error(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	exist, err := configmanager.DeleteMachineGroup(groupName)

	if err != nil {
		c.JSON(500, common.Error(common.InternalServerError, err.Error()))
	} else if !exist {
		c.JSON(404, common.Error(common.MachineGroupNotExist, fmt.Sprintf("Machine group %s doesn't exist.", groupName)))
	} else {
		c.JSON(200, common.Accept("Delete machine group success", nil))
	}
}

func GetMachineGroup(c *gin.Context) {
	groupName := c.Query("groupName")
	if groupName == "" {
		c.JSON(400, common.Error(common.BadRequest, fmt.Sprintf("Need parameter %s.", "groupName")))
		return
	}

	machineGroup, err := configmanager.GetMachineGroup(groupName)

	if err != nil {
		c.JSON(500, common.Error(common.InternalServerError, err.Error()))
	} else if machineGroup == nil {
		c.JSON(404, common.Error(common.MachineGroupNotExist, fmt.Sprintf("Machine group %s doesn't exist.", groupName)))
	} else {
		c.JSON(200, common.Accept("Get machine group success", gin.H{"machineGroup": machineGroup}))
	}
}

func ListMachineGroups(c *gin.Context) {
	machineGroupList, err := configmanager.GetAllMachineGroup()

	if err != nil {
		c.JSON(500, common.Error(common.InternalServerError, err.Error()))
	} else {
		c.JSON(200, common.Accept("Get machine group list success", gin.H{"machineGroupList": machineGroupList}))
	}
}
