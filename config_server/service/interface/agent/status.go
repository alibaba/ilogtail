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

package agent

import (
	"fmt"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/manager"
	"github.com/gin-gonic/gin"
)

func HeartBeat(c *gin.Context) {
	id := c.PostForm("instance_id")
	tags := c.PostFormMap("tags")
	if id == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "instance_id")))
		return
	}

	err := manager.AgentManager().HeartBeat(id, c.ClientIP(), tags)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Heartbeat success", nil))
	}
}

func RunningStatus(c *gin.Context) {
	id := c.PostForm("instance_id")
	status := c.PostFormMap("status")
	if id == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "instance_id")))
		return
	}

	err := manager.AgentManager().RunningStatus(id, status)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Send status success", nil))
	}
}

func Alarm(c *gin.Context) {
	id := c.PostForm("instance_id")
	alarmType := c.PostForm("alarm_type")
	alarmMessage := c.PostForm("alarm_message")
	if id == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "instance_id")))
		return
	}

	err := manager.AgentManager().Alarm(id, alarmType, alarmMessage)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Alarm success", nil))
	}
}
