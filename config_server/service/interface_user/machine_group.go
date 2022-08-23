package user

import (
	"fmt"

	"github.com/alibaba/ilogtail/config_server/service/common"
	agentmanager "github.com/alibaba/ilogtail/config_server/service/manager_agent"
	configmanager "github.com/alibaba/ilogtail/config_server/service/manager_config"
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

	exist, err := configmanager.CreateMachineGroup(groupName, groupTag, description)

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

	exist, err := configmanager.UpdateMachineGroup(groupName, groupTag, description)

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

	exist, err := configmanager.DeleteMachineGroup(groupName)

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

	machineGroup, err := configmanager.GetMachineGroup(groupName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if machineGroup == nil {
		c.JSON(common.ErrorResponse(common.MachineGroupNotExist, fmt.Sprintf("Machine group %s doesn't exist.", groupName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Get machine group success", gin.H{"machineGroup": machineGroup}))
	}
}

func ListMachineGroups(c *gin.Context) {
	machineGroupList, err := configmanager.GetAllMachineGroup()

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

	machineList, err := agentmanager.GetMachineList(groupName)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if machineList == nil {
		c.JSON(common.ErrorResponse(common.MachineGroupNotExist, fmt.Sprintf("Machine group %s doesn't exist.", groupName)))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Get machine list success", gin.H{"machineList": machineList}))
	}
}

func ApplyConfigToMachineGroup(c *gin.Context) {
}

func RemoveConfigFromMachineGroup(c *gin.Context) {
}

func GetAppliedConfigs(c *gin.Context) {
}
