package agent

import (
	"fmt"

	"github.com/alibaba/ilogtail/config_server/service/common"
	agentmanager "github.com/alibaba/ilogtail/config_server/service/manager_agent"
	"github.com/gin-gonic/gin"
)

func HeartBeat(c *gin.Context) {
	instance_id := c.PostForm("instance_id")
	tags := c.PostFormMap("tags")
	if instance_id == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "instance_id")))
		return
	}

	err := agentmanager.HeartBeat(instance_id, c.ClientIP(), tags)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Heartbeat success", nil))
	}
}

func RunningStatus(c *gin.Context) {
	instance_id := c.PostForm("instance_id")
	status := c.PostFormMap("status")
	if instance_id == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "instance_id")))
		return
	}

	err := agentmanager.RunningStatus(instance_id, status)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Heartbeat success", nil))
	}
}

func Alarm(c *gin.Context) {
}
