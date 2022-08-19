package user

import (
	agentmanager "github.com/alibaba/ilogtail/config_server/config_server_service/agent_manager"
	"github.com/alibaba/ilogtail/config_server/config_server_service/common"
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
