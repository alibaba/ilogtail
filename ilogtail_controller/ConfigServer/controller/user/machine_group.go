package user

import (
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/common"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/store"
	"github.com/gin-gonic/gin"
)

func CreateMachineGroup(c *gin.Context) {
	groupName := c.PostForm("groupName")
	groupTag := c.PostForm("groupTag")
	description := c.PostForm("description")

	myMachineGroups := store.GetStore().MachineGroup()

	ok, err := myMachineGroups.Has(groupName)
	if err != nil {
		c.JSON(500, common.Error(common.InternalServerError, err.Error()))
	} else if ok {
		c.JSON(400, common.Error(common.MachineGroupAlreadyExist, ""))
	} else {
		machineGroup := model.NewMachineGroup(groupName, description, groupTag)
		myMachineGroups.Add(machineGroup)
		c.JSON(200, common.Accept("Add machine group success"))
	}
}

func UpdateMachineGroup(c *gin.Context) {
}

func DeleteMachineGroup(c *gin.Context) {
}

func GetMachineGroup(c *gin.Context) {
}

func ListMachineGroups(c *gin.Context) {
}
