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
	proto "github.com/alibaba/ilogtail/config_server/service/proto"
	"github.com/gin-gonic/gin"
	"github.com/gin-gonic/gin/binding"
)

func HeartBeat(c *gin.Context) {
	req := proto.HeartBeatRequest{}
	res := &proto.HeartBeatResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.AgentId == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "AgentId")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	err = manager.AgentManager().HeartBeat(req.AgentId, req.AgentVersion, req.Ip, req.Tags, req.AgentVersion, req.StartupTime)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Send heartbeat success"
		c.ProtoBuf(common.Accept.Status, res)
	}
}

func RunningStatistics(c *gin.Context) {
	req := proto.RunningStatisticsRequest{}
	res := &proto.RunningStatisticsResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.AgentId == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "AgentId")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	err = manager.AgentManager().RunningStatistics(req.AgentId, req.RunningDetails)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Send running status success"
		c.ProtoBuf(common.Accept.Status, res)
	}
}

func Alarm(c *gin.Context) {
	req := proto.AlarmRequest{}
	res := &proto.AlarmResponse{}

	err := c.ShouldBindBodyWith(&req, binding.ProtoBuf)
	if err != nil {
		res.Code = common.BadRequest.Code
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}
	res.ResponseId = req.RequestId

	if req.AgentId == "" {
		res.Code = common.BadRequest.Code
		res.Message = fmt.Sprintf("Need parameter %s.", "AgentId")
		c.ProtoBuf(common.BadRequest.Status, res)
		return
	}

	err = manager.AgentManager().Alarm(req.AgentId, req.Type, req.Detail)

	if err != nil {
		res.Code = common.InternalServerError.Code
		res.Message = err.Error()
		c.ProtoBuf(common.InternalServerError.Status, res)
	} else {
		res.Code = common.Accept.Code
		res.Message = "Alarm success"
		c.ProtoBuf(common.Accept.Status, res)
	}
}
