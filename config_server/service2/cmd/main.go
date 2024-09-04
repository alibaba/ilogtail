package main

import (
	"config-server2/internal/config"
	"config-server2/internal/router"
	"github.com/gin-gonic/gin"
)

func main() {
	r := gin.Default()
	router.InitAllRouter(r)
	err := r.Run(config.ServerConfigInstance.Address)
	if err != nil {
		panic(err)
	}
}
