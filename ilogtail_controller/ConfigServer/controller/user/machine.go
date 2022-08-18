package user

import (
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/common"
	ilogtailmanager "github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/service/ilogtail_manager"
	"github.com/gin-gonic/gin"
)

func ListAllMachines(c *gin.Context) {
	machineList, err := ilogtailmanager.GetAllMachine()

	if err != nil {
		c.JSON(500, common.Error(common.InternalServerError, err.Error()))
	} else {
		c.JSON(200, common.Accept("Get machine list success", gin.H{"machineList": machineList}))
	}
}
