package user

import (
	"net/http"

	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/model"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/store"
	"github.com/gin-gonic/gin"
)

func CreateMachineGroup(c *gin.Context) {
	groupName := c.PostForm("groupName")
	groupTag := c.PostForm("groupTag")
	description := c.PostForm("description")

	myMachineGroups := store.GetStore().MachineGroup()

	if myMachineGroups.Has(groupName) {
		c.String(400, "Machine group already exists.")
	} else {
		machineGroup := model.NewMachineGroup(groupName, description, groupTag)
		myMachineGroups.Add(machineGroup)
		c.String(http.StatusOK, "Add machine group success")
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
