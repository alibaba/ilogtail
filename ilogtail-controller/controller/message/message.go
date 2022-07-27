package message

import "github.com/gin-gonic/gin"

func MessageMain() {
	engine := gin.Default()
	machineGroup := engine.Group("/machine")
	{
		machineGroup.GET("/register/:uid", machineRegister)
		machineGroup.POST("/addConfig/", addConfigToMachine)
		machineGroup.POST("/delConfig/", delConfigFromMachine)
		machineGroup.GET("/list/", getMachineList)
	}
	localGroup := engine.Group("/local")
	{
		localGroup.POST("/addConfig/", addConfig)
		localGroup.GET("/delConfig/:name", delConfig)
	}
	engine.Run("127.0.0.1:8899")
}

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
