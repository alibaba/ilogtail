package service

import (
	"github.com/alibaba/ilogtail/ilogtail-controller/controller/manager/store"
	"github.com/gin-gonic/gin"
)

func MessageMain() {
	engine := gin.Default()
	machineGroup := engine.Group("/fromMachine")
	{
		machineGroup.GET("/register/:instance_id", machineRegister)
		machineGroup.POST("/state", machineState)
		machineGroup.POST("/configList", machineConfigList)
	}
	uiGroup := engine.Group("/fromUI")
	{
		dataGroup := uiGroup.Group("/data")
		{
			dataGroup.GET("/configList/", getConfigList)
			dataGroup.GET("/machineList/", getMachineList)
			dataGroup.GET("/machineGroupList/", getMachineGroupList)
		}

		localGroup := uiGroup.Group("/local")
		{
			localGroup.POST("/addConfig/", addConfig)
			localGroup.POST("/modConfig/", modConfig)
			localGroup.GET("/delConfig/:name", delConfig)

			localGroup.POST("/modMachine/", modMachine)
			localGroup.GET("/delMachine/:instance_id", delMachine)

			localGroup.POST("/addMachineGroup/", addMachineGroup)
			localGroup.POST("/modMachineGroup/", modMachineGroup)
			localGroup.GET("/delMachineGroup/:name", delMachineGroup)

			localGroup.POST("/addMachineToGroup/", addMachineToGroup)
			localGroup.POST("/delMachineFromGroup/", delMachineFromGroup)
			localGroup.POST("/addConfigToGroup/", addConfigToGroup)
			localGroup.POST("/delConfigFromGroup/", delConfigFromGroup)
		}

		toMachineGroup := uiGroup.Group("/toMachine")
		{
			toMachineGroup.POST("/addConfigToMachine/", addConfigToMachine)
			toMachineGroup.POST("/delConfigFromMachine/", delConfigFromMachine)
		}
	}
	engine.Run("127.0.0.1:" + store.GetMyBaseSetting().Port)
}

/*
func GinTest() {
	// 创建一个默认的路由引擎
	engine := gin.Default()
	// 注册路由
	engine.GET("/test/:name", func(context *gin.Context) {
		// 接收参数
		name := context.Param("name")
		context.JSON(200, gin.H{"msg": "success", "name": name})
	})
	engine.GET("/test/:name/:age", func(context *gin.Context) {
		// 接收参数
		name := context.Param("name")
		age := context.Param("age")
		context.JSON(200, gin.H{
			"msg":   "success",
			"name":  name,
			"phone": age,
		})
	})
	engine.GET("/test/:name/:age/:height", func(context *gin.Context) {
		// 接收参数
		name := context.Param("name")
		age := context.Param("age")
		height := context.Param("height")
		context.JSON(200, gin.H{
			"msg":    "success",
			"name":   name,
			"phone":  age,
			"height": height,
		})
	})
	_ = engine.Run()
}
*/
