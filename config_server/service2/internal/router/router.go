package router

import (
	"config-server2/internal/config"
	"config-server2/internal/server_agent/handler"
	"github.com/gin-gonic/gin"
)

func initUserRouter(engine *gin.Engine) {
	engine.Group("/User")
}

func initAgentRouter(engine *gin.Engine) {
	agentRouter := engine.Group("/Agent")
	{
		agentRouter.POST("/Heartbeat", handler.HeartBeat)
		agentRouter.POST("/FetchPipelineConfig", handler.FetchPipelineConfig)
		//agent有bug暂时不开启此路由
		//agentRouter.POST("/FetchProcessConfig", handler.FetchProcessConfig)
	}
}

func InitAllRouter() {
	router := gin.Default()
	handler.CheckAgentExist()
	initUserRouter(router)
	initAgentRouter(router)
	err := router.Run(config.ServerConfigInstance.Address)
	if err != nil {
		panic(err)
	}
}
