package service

import (
	"net/http"
	"time"

	"github.com/alibaba/ilogtail/ilogtail-controller/controller/manager"
	"github.com/gin-gonic/gin"
)

func getConfigList(c *gin.Context) {
	data := gin.H{}
	c.JSON(http.StatusOK, data)
}

func getMachineList(c *gin.Context) {
	queryTime := time.Now().Format("2006-01-02 15:04:05")
	data := manager.GetMachineList(queryTime)
	c.JSON(http.StatusOK, data)
}

func getMachineGroupList(c *gin.Context) {
	data := gin.H{}
	c.JSON(http.StatusOK, data)
}
