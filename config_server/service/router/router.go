package router

import (
	agent "github.com/alibaba/ilogtail/config_server/service/interface_agent"
	user "github.com/alibaba/ilogtail/config_server/service/interface_user"
	"github.com/alibaba/ilogtail/config_server/service/setting"
	"github.com/gin-gonic/gin"
)

func InitRouter() {
	router := gin.Default()

	if setting.GetSetting().Identity == "master" {
		initUserRouter(router)
	}
	initAgentRouter(router)

	router.Run(setting.GetSetting().Ip + ":" + setting.GetSetting().Port)
}

func initUserRouter(router *gin.Engine) {
	userGroup := router.Group("/User")
	{
		userGroup.POST("/CreateMachineGroup", user.CreateMachineGroup)
		userGroup.PUT("/UpdateMachineGroup", user.UpdateMachineGroup)
		userGroup.DELETE("/DeleteMachineGroup", user.DeleteMachineGroup)
		userGroup.GET("/GetMachineGroup", user.GetMachineGroup)
		userGroup.GET("/ListMachineGroups", user.ListMachineGroups)

		userGroup.POST("/CreateConfig", user.CreateConfig)
		userGroup.PUT("/UpdateConfig", user.UpdateConfig)
		userGroup.DELETE("/DeleteConfig", user.DeleteConfig)
		userGroup.GET("/GetConfig", user.GetConfig)
		userGroup.GET("/ListAllConfigs", user.ListAllConfigs)

		userGroup.PUT("/ApplyConfigToMachineGroup", user.ApplyConfigToMachineGroup)
		userGroup.DELETE("/RemoveConfigFromMachineGroup", user.RemoveConfigFromMachineGroup)
		userGroup.GET("/GetAppliedConfigs", user.GetAppliedConfigs)
		userGroup.GET("/GetAppliedMachineGroups", user.GetAppliedMachineGroups)
		userGroup.GET("/ListMachines", user.ListMachines)
	}
}

func initAgentRouter(router *gin.Engine) {
	agentGroup := router.Group("/Agent")
	{
		agentGroup.POST("/HeartBeat", agent.HeartBeat)
		agentGroup.POST("/RunningStatus", agent.RunningStatus)
		agentGroup.POST("/Alarm", agent.Alarm)

		agentGroup.POST("/CheckConfigList", agent.CheckConfigList)
	}
}
