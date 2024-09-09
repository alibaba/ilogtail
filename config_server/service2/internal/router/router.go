package router

import (
	"config-server2/internal/handler"
	"github.com/gin-gonic/gin"
)

func initUserRouter(router *gin.Engine) {
	userRouter := router.Group("/User")
	userRouter.Use(gin.Logger())
	{
		userRouter.POST("/CreateAgentGroup", handler.CreateAgentGroup)
		userRouter.PUT("/UpdateAgentGroup", handler.UpdateAgentGroup)
		userRouter.DELETE("/DeleteAgentGroup", handler.DeleteAgentGroup)
		userRouter.GET("/GetAgentGroup", handler.GetAgentGroup)
		userRouter.GET("/ListAgentGroups", handler.ListAgentGroups)
		userRouter.GET("/ListAgents", handler.ListAgentsInGroup)
		userRouter.GET("/GetAppliedAgentGroupsWithPipelineConfig", handler.GetAppliedAgentGroupsWithPipelineConfig)
		userRouter.GET("/GetAppliedAgentGroupsWithInstanceConfig", handler.GetAppliedAgentGroupsWithInstanceConfig)

		userRouter.POST("/CreatePipelineConfig", handler.CreatePipelineConfig)
		userRouter.PUT("/UpdatePipelineConfig", handler.UpdatePipelineConfig)
		userRouter.DELETE("/DeletePipelineConfig", handler.DeletePipelineConfig)
		userRouter.GET("/GetPipelineConfig", handler.GetPipelineConfig)
		userRouter.GET("/ListPipelineConfigs", handler.ListPipelineConfigs)
		userRouter.POST("/ApplyPipelineConfigToAgentGroup", handler.ApplyPipelineConfigToAgentGroup)
		userRouter.DELETE("/RemovePipelineConfigFromAgentGroup", handler.RemovePipelineConfigFromAgentGroup)
		userRouter.GET("/GetAppliedPipelineConfigsForAgentGroup", handler.GetAppliedPipelineConfigsForAgentGroup)

		userRouter.POST("/CreateInstanceConfig", handler.CreateInstanceConfig)
		userRouter.PUT("/UpdateInstanceConfig", handler.UpdateInstanceConfig)
		userRouter.DELETE("/DeleteInstanceConfig", handler.DeleteInstanceConfig)
		userRouter.GET("/GetInstanceConfig", handler.GetInstanceConfig)
		userRouter.GET("/ListInstanceConfigs", handler.ListInstanceConfigs)
		userRouter.POST("/ApplyInstanceConfigToAgentGroup", handler.ApplyInstanceConfigToAgentGroup)
		userRouter.DELETE("/RemoveInstanceConfigFromAgentGroup", handler.RemoveInstanceConfigFromAgentGroup)
		userRouter.GET("/GetAppliedInstanceConfigsForAgentGroup", handler.GetAppliedInstanceConfigsForAgentGroup)
	}

}

func initAgentRouter(router *gin.Engine) {
	agentRouter := router.Group("/Agent")
	{
		agentRouter.POST("/Heartbeat", handler.HeartBeat)
		agentRouter.POST("/FetchPipelineConfig", handler.FetchPipelineConfig)
		//agent有bug暂时不开启此路由
		//agentRouter.POST("/FetchProcessConfig", handler.FetchProcessConfig)
	}
}

func InitAllRouter(router *gin.Engine) {
	handler.CheckAgentExist()
	initAgentRouter(router)
	initUserRouter(router)
}
