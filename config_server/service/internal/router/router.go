package router

import (
	"config-server/internal/handler"
	"github.com/gin-gonic/gin"
)

func initUserRouter(router *gin.Engine) {
	userRouter := router.Group("/User")
	userRouter.Use(gin.Logger())
	{
		userRouter.POST("/CreateAgentGroup", handler.CreateAgentGroup)
		userRouter.POST("/UpdateAgentGroup", handler.UpdateAgentGroup)
		userRouter.POST("/DeleteAgentGroup", handler.DeleteAgentGroup)
		userRouter.POST("/GetAgentGroup", handler.GetAgentGroup)
		userRouter.POST("/ListAgentGroups", handler.ListAgentGroups)
		userRouter.POST("/ListAgents", handler.ListAgentsInGroup)
		userRouter.POST("/GetAppliedPipelineConfigsForAgentGroup", handler.GetAppliedPipelineConfigsForAgentGroup)
		userRouter.POST("/GetAppliedInstanceConfigsForAgentGroup", handler.GetAppliedInstanceConfigsForAgentGroup)

		userRouter.POST("/CreatePipelineConfig", handler.CreatePipelineConfig)
		userRouter.POST("/UpdatePipelineConfig", handler.UpdatePipelineConfig)
		userRouter.POST("/DeletePipelineConfig", handler.DeletePipelineConfig)
		userRouter.POST("/GetPipelineConfig", handler.GetPipelineConfig)
		userRouter.POST("/ListPipelineConfigs", handler.ListPipelineConfigs)
		userRouter.POST("/ApplyPipelineConfigToAgentGroup", handler.ApplyPipelineConfigToAgentGroup)
		userRouter.POST("/RemovePipelineConfigFromAgentGroup", handler.RemovePipelineConfigFromAgentGroup)
		userRouter.POST("/GetAppliedAgentGroupsWithPipelineConfig", handler.GetAppliedAgentGroupsWithPipelineConfig)
		userRouter.POST("/GetPipelineConfigStatusList", handler.GetPipelineConfigStatusList)

		userRouter.POST("/CreateInstanceConfig", handler.CreateInstanceConfig)
		userRouter.POST("/UpdateInstanceConfig", handler.UpdateInstanceConfig)
		userRouter.POST("/DeleteInstanceConfig", handler.DeleteInstanceConfig)
		userRouter.POST("/GetInstanceConfig", handler.GetInstanceConfig)
		userRouter.POST("/ListInstanceConfigs", handler.ListInstanceConfigs)
		userRouter.POST("/ApplyInstanceConfigToAgentGroup", handler.ApplyInstanceConfigToAgentGroup)
		userRouter.POST("/RemoveInstanceConfigFromAgentGroup", handler.RemoveInstanceConfigFromAgentGroup)
		userRouter.POST("/GetAppliedAgentGroupsWithInstanceConfig", handler.GetAppliedAgentGroupsWithInstanceConfig)
		userRouter.POST("/GetInstanceConfigStatusList", handler.GetInstanceConfigStatusList)
	}

}

func initAgentRouter(router *gin.Engine) {
	agentRouter := router.Group("/Agent")
	agentRouter.Use(gin.Logger())
	{
		agentRouter.POST("/Heartbeat", handler.HeartBeat)
		agentRouter.POST("/FetchPipelineConfig", handler.FetchPipelineConfig)
		//agent有bug暂时不开启此路由
		//agentRouter.POST("/FetchProcessConfig", handler.FetchProcessConfig)
	}
}

func initTest(router *gin.Engine) {
	testRouter := router.Group("/Test")
	{
		testRouter.GET("", func(c *gin.Context) {
			c.JSON(200, gin.H{
				"config-server": "connect success",
			})
		})
	}
}

func InitAllRouter(router *gin.Engine) {
	handler.CheckAgentExist()
	handler.AppliedOrRemoveConfigForAgentGroup()
	initAgentRouter(router)
	initUserRouter(router)
	initTest(router)
}
