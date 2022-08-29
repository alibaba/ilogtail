// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package router

import (
	"github.com/alibaba/ilogtail/config_server/service/interface/agent"
	"github.com/alibaba/ilogtail/config_server/service/interface/user"
	"github.com/alibaba/ilogtail/config_server/service/setting"
	"github.com/gin-gonic/gin"
)

func InitRouter() {
	router := gin.Default()

	initUserRouter(router)
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

		agentGroup.POST("/PullConfigUpdates", agent.PullConfigUpdates)
	}
}
