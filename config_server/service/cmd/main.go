package main

import (
	"config-server/internal/config"
	"config-server/internal/router"
	"github.com/gin-gonic/gin"
)

func main() {
	r := gin.New()
	router.InitAllRouter(r)
	err := r.Run(config.ServerConfigInstance.Address)
	if err != nil {
		panic(err)
	}
}
