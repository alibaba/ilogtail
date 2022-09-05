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

func PullConfigUpdates(c *gin.Context) {
	id := c.PostForm("instance_id")
	configs := c.PostFormMap("configs")
	if id == "" {
		c.JSON(common.ErrorResponse(common.BadRequest, fmt.Sprintf("Need parameter %s.", "instance_id")))
		return
	}
	if configs == nil {
		configs = make(map[string]string)
	}

	result, configExist, agentExist, err := manager.ConfigManager().PullConfigUpdates(id, configs)

	if err != nil {
		c.JSON(common.ErrorResponse(common.InternalServerError, err.Error()))
	} else if !agentExist {
		c.JSON(common.ErrorResponse(common.AgentNotExist, fmt.Sprintf("Agent %s doesn't exist.", id)))
	} else if !configExist {
		c.JSON(common.ErrorResponse(common.ConfigNotExist, fmt.Sprintf("Find config failed.")))
	} else {
		c.JSON(common.AcceptResponse(common.Accept, "Check config success", gin.H{"config_check_result": result}))
	}
}
