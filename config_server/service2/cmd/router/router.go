package router

import (
	"config-server2/internal/handler"
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
	err := router.Run("127.0.0.1:9090")
	if err != nil {
		return
	}
}
