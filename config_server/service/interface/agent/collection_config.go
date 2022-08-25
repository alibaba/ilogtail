package agent

import (
	"fmt"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/manager"
	"github.com/gin-gonic/gin"
)

func CheckConfigList(c *gin.Context) {
	id := c.PostForm("instance_id")
	configs := c.PostFormMap("configs")
	if id == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "instance_id")))
		return
	}
	if configs == nil {
		configs = make(map[string]string)
	}

	result, configExist, machineExist, err := manager.ConfigManager().CheckConfigList(id, configs)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if !machineExist {
		c.JSON(common.ErrorResponse(common.MachineNotExist, fmt.Sprintf("Machine %s doesn't exist.", id)))
	} else if !configExist {
		c.JSON(common.ErrorResponse(common.ConfigNotExist, fmt.Sprintf("Find config failed.")))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Check config success", gin.H{"config_check_result": result}))
	}
}
