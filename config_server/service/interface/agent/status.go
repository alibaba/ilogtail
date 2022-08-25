package agent

import (
	"fmt"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/manager"
	"github.com/gin-gonic/gin"
)

func HeartBeat(c *gin.Context) {
	id := c.PostForm("instance_id")
	tags := c.PostFormMap("tags")
	if id == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "instance_id")))
		return
	}

	err := manager.AgentManager().HeartBeat(id, c.ClientIP(), tags)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Heartbeat success", nil))
	}
}

func RunningStatus(c *gin.Context) {
	id := c.PostForm("instance_id")
	status := c.PostFormMap("status")
	if id == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "instance_id")))
		return
	}

	err := manager.AgentManager().RunningStatus(id, status)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Send status success", nil))
	}
}

func Alarm(c *gin.Context) {
	id := c.PostForm("instance_id")
	alarmType := c.PostForm("alarm_type")
	alarmMessage := c.PostForm("alarm_message")
	if id == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "instance_id")))
		return
	}

	err := manager.AgentManager().Alarm(id, alarmType, alarmMessage)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Alarm success", nil))
	}
}
