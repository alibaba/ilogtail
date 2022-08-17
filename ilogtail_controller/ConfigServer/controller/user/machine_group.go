package user

import (
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/common"
	configmanager "github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/service/config_manager"
	"github.com/gin-gonic/gin"
)

func CreateMachineGroup(c *gin.Context) {
	groupName := c.PostForm("groupName")
	groupTag := c.PostForm("groupTag")
	description := c.PostForm("description")

	conflict, err := configmanager.CreateMachineGroup(groupName, groupTag, description)

	if err != nil {
		c.JSON(500, common.Error(common.InternalServerError, err.Error()))
	} else if conflict {
		c.JSON(400, common.Error(common.MachineGroupAlreadyExist, ""))
	} else {
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
