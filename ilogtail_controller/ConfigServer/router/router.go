package router

import (
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/controller/ilogtail"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/controller/user"
	"github.com/alibaba/ilogtail/ilogtail_controller/ConfigServer/setting"
	"github.com/gin-gonic/gin"
)

func InitRouter() {
	router := gin.Default()

	if setting.GetSetting().Identity == "master" {
		initUserRouter(router)
	}
	initIlogtailRouter(router)

	router.Run("127.0.0.1:" + setting.GetSetting().Port)
}

func initUserRouter(router *gin.Engine) {
	userGroup := router.Group("/USER")
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
		userGroup.GET("/ListConfigs", user.ListConfigs)

		userGroup.PUT("/ApplyConfigToMachineGroup", user.ApplyConfigToMachineGroup)
		userGroup.DELETE("/RemoveConfigFromMachineGroup", user.RemoveConfigFromMachineGroup)
		userGroup.GET("/GetAppliedConfigs", user.GetAppliedConfigs)
		userGroup.GET("/GetAppliedMachineGroups", user.GetAppliedMachineGroups)
		userGroup.GET("/ListMachines", user.ListMachines)

		userGroup.GET("/ListAllMachines", user.ListAllMachines)
	}
}

func initIlogtailRouter(router *gin.Engine) {
	ilogtailGroup := router.Group("/ilogtail")
	{
		ilogtailGroup.GET("/HeartBeat", ilogtail.HeartBeat)
		ilogtailGroup.POST("/RunTimeStatus", ilogtail.RunTimeStatus)
		ilogtailGroup.POST("/Alarm", ilogtail.Alarm)

		ilogtailGroup.POST("/CheckConfigList", ilogtail.CheckConfigList)
	}
}
