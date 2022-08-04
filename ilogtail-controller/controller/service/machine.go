package service

import (
	"net/http"
	"time"

	"github.com/alibaba/ilogtail/ilogtail-controller/controller/manager"

	"github.com/gin-gonic/gin"
)

type MACO struct {
	Machines []string `form:"machines" json:"machines" uri:"machines" xml:"machines" binding:"required"`
	Configs  []string `form:"configs" json:"configs" uri:"configs" xml:"configs" binding:"required"`
}

func machineRegister(c *gin.Context) {
	id := c.Param("instance_id")
	heart := time.Now().Format("2006-01-02 15:04:05")
	ip := c.ClientIP()
	isNew := manager.RegisterMachine(id, heart, ip)
	if isNew {
		c.String(http.StatusOK, "Register successfully!")
	} else {
		c.String(http.StatusOK, "Heart Beat successfully!")
	}
}

func machineState(c *gin.Context) {

}

func machineConfigList(c *gin.Context) {

}
