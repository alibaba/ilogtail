package user

import (
	"github.com/alibaba/ilogtail/config_server/service/common"
	agentmanager "github.com/alibaba/ilogtail/config_server/service/manager_agent"
	"github.com/gin-gonic/gin"
)

func ListAllMachines(c *gin.Context) {
	machineList, err := agentmanager.GetAllMachine()

	if err != nil {
		c.JSON(500, common.Error(common.InternalServerError, err.Error()))
	} else {
		c.JSON(200, common.Accept("Get machine list success", gin.H{"machineList": machineList}))
	}
}
