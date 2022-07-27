package message

import (
	"net/http"

	"github.com/alibaba/ilogtail/ilogtail-controller/controller/manager"

	"github.com/gin-gonic/gin"
)

func addConfig(c *gin.Context) {
	configName := c.PostForm("name")
	configPath := c.PostForm("path")
	configDescription := c.PostForm("description")
	manager.AddConfig(manager.NewConfig(configName, configPath, configDescription))
	manager.Update()
	c.String(http.StatusOK, "Add config success")
}

func delConfig(c *gin.Context) {
	configName := c.Param("name")
	manager.DelConfig(configName)
	manager.Update()
	c.String(http.StatusOK, "Add config success")
}
