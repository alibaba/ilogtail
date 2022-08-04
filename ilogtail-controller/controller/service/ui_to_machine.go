package service

import (
	"net/http"

	"github.com/alibaba/ilogtail/ilogtail-controller/controller/manager"
	"github.com/gin-gonic/gin"
)

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
