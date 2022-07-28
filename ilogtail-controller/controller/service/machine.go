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

func addConfigToMachine(c *gin.Context) {
	// 声明接收的变量
	var mc MACO
	// Bind()默认解析并绑定form格式
	// 根据请求头中content-type自动推断
	if err := c.Bind(&mc); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	for _, m := range mc.Machines {
		for _, co := range mc.Configs {
			ip, status := manager.AddConfigToMachine(m, co)
			c.JSON(status, gin.H{"target": ip})
		}
	}
}

func delConfigFromMachine(c *gin.Context) {
	// 声明接收的变量
	var mc MACO
	// Bind()默认解析并绑定form格式
	// 根据请求头中content-type自动推断
	if err := c.Bind(&mc); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	for _, m := range mc.Machines {
		for _, co := range mc.Configs {
			ip, status := manager.DelConfigFromMachine(m, co)
			c.JSON(status, gin.H{"target": ip})
		}
	}
	c.JSON(http.StatusOK, gin.H{"status": "200"})
}

func getMachineList(c *gin.Context) {
	queryTime := time.Now().Format("2006-01-02 15:04:05")
	data := manager.GetMachineList(queryTime)
	c.JSON(http.StatusOK, data)
}
