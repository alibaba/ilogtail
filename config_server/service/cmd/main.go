package main

import (
	"config-server/config"
	"config-server/router"
	"github.com/gin-gonic/gin"
	"os"
)

func main() {
	env := os.Getenv("GO_ENV")
	var r *gin.Engine
	if env == "prod" {
		gin.SetMode(gin.ReleaseMode)
		r = gin.New()
	} else {
		gin.SetMode(gin.DebugMode)
		r = gin.Default()
	}
	router.InitAllRouter(r, env)
	err := r.Run(config.ServerConfigInstance.Address)
	if err != nil {
		panic(err)
	}
}
