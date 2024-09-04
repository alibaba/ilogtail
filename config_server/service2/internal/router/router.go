package router

import (
	"config-server2/internal/handler"
	"github.com/gin-gonic/gin"
)

func initUserRouter(engine *gin.Engine) {
	userRouter := engine.Group("/User")
	{
		userRouter.POST("/CreateAgentGroup", handler.CreateAgentGroup)
		userRouter.PUT("/UpdateAgentGroup", handler.UpdateAgentGroup)
		userRouter.DELETE("/DeleteAgentGroup", handler.DeleteAgentGroup)
		userRouter.POST("/GetAgentGroup", handler.GetAgentGroup)
		userRouter.POST("/ListAgentGroups", handler.ListAgentGroups)
		userRouter.POST("/ListAgents", handler.ListAgentsInGroup)
		userRouter.POST("/GetAppliedAgentGroupsWithPipelineConfig", handler.GetAppliedAgentGroupsWithPipelineConfig)
		userRouter.POST("/GetAppliedAgentGroupsWithInstanceConfig", handler.GetAppliedAgentGroupsWithInstanceConfig)

		userRouter.POST("/CreatePipelineConfig", handler.CreatePipelineConfig)
		userRouter.PUT("/UpdatePipelineConfig", handler.UpdatePipelineConfig)
		userRouter.DELETE("/DeletePipelineConfig", handler.DeletePipelineConfig)
		userRouter.POST("/GetPipelineConfig", handler.GetPipelineConfig)
		userRouter.POST("/ListPipelineConfigs", handler.ListPipelineConfigs)
		userRouter.PUT("/ApplyPipelineConfigToAgentGroup", handler.ApplyPipelineConfigToAgentGroup)
		userRouter.DELETE("/RemovePipelineConfigFromAgentGroup", handler.RemovePipelineConfigFromAgentGroup)
		userRouter.POST("/GetAppliedPipelineConfigsForAgentGroup", handler.GetAppliedPipelineConfigsForAgentGroup)

		userRouter.POST("/CreateInstanceConfig", handler.CreateInstanceConfig)
		userRouter.PUT("/UpdateInstanceConfig", handler.UpdateInstanceConfig)
		userRouter.DELETE("/DeleteInstanceConfig", handler.DeleteInstanceConfig)
		userRouter.POST("/GetInstanceConfig", handler.GetInstanceConfig)
		userRouter.POST("/ListInstanceConfigs", handler.ListInstanceConfigs)
		userRouter.PUT("/ApplyInstanceConfigToAgentGroup", handler.ApplyInstanceConfigToAgentGroup)
		userRouter.DELETE("/RemoveInstanceConfigFromAgentGroup", handler.RemoveInstanceConfigFromAgentGroup)
		userRouter.POST("/GetAppliedInstanceConfigsForAgentGroup", handler.GetAppliedInstanceConfigsForAgentGroup)
	}

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

func InitAllRouter(router *gin.Engine) {
	handler.CheckAgentExist()
	initUserRouter(router)
	initAgentRouter(router)
}
