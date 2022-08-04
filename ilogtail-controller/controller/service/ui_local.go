package service

import (
	"net/http"

	"github.com/alibaba/ilogtail/ilogtail-controller/controller/manager"

	"github.com/gin-gonic/gin"
)

func addConfig(c *gin.Context) {
	configName := c.PostForm("name")
	configPath := c.PostForm("path")
	configDescription := c.PostForm("description")
	manager.AddLocalConfig(configName, configPath, configDescription)
	c.String(http.StatusOK, "Add config success")
}

func modConfig(c *gin.Context) {

	c.String(http.StatusOK, "Mod config success")
}

func delConfig(c *gin.Context) {
	configName := c.Param("name")
	manager.DelLocalConfig(configName)
	c.String(http.StatusOK, "Del config success")
}

func modMachine(c *gin.Context) {

	c.String(http.StatusOK, "Mod machine success")
}

func delMachine(c *gin.Context) {

	c.String(http.StatusOK, "Del machine success")
}

func addMachineGroup(c *gin.Context) {

	c.String(http.StatusOK, "Add machine group success")
}

func modMachineGroup(c *gin.Context) {

	c.String(http.StatusOK, "Mod machine group success")
}

func delMachineGroup(c *gin.Context) {

	c.String(http.StatusOK, "Del machine group success")
}

func addMachineToGroup(c *gin.Context) {

	c.String(http.StatusOK, "Add machine to machine group success")
}

func delMachineFromGroup(c *gin.Context) {

	c.String(http.StatusOK, "Del machine from machine group success")
}

func addConfigToGroup(c *gin.Context) {

	c.String(http.StatusOK, "Add config to machine group success")
}

func delConfigFromGroup(c *gin.Context) {

	c.String(http.StatusOK, "Del config from machine group success")
}
