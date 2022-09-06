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

package model

import (
	"fmt"
	"strconv"

	proto "github.com/alibaba/ilogtail/config_server/service/proto"
)

type Agent struct {
	AgentId             string            `json:"AgentId"`
	AgentType           string            `json:"AgentType"`
	Ip                  string            `json:"Ip"`
	Version             string            `json:"Version"`
	RunningStatus       string            `json:"RunningStatus"`
	StartupTime         int64             `json:"StartupTime"`
	LatestHeartbeatTime int64             `json:"LatestHeartbeatTime"`
	Tags                map[string]string `json:"Tags"`
	RunningDetails      map[string]string `json:"RunningDetails"`
}

func (a *Agent) ToProto() *proto.Agent {
	pa := new(proto.Agent)
	pa.AgentId = a.AgentId
	pa.AgentType = a.AgentType
	pa.Ip = a.Ip
	pa.Version = a.Version
	pa.RunningStatus = a.RunningStatus
	pa.StartupTime = a.StartupTime
	pa.LatestHeartbeatTime = a.LatestHeartbeatTime
	pa.Tags = a.Tags
	pa.RunningDetails = a.RunningDetails
	return pa
}

func (a *Agent) ParseProto(pa *proto.Agent) {
	a.AgentId = pa.AgentId
	a.AgentType = pa.AgentType
	a.Ip = pa.Ip
	a.Version = pa.Version
	a.RunningStatus = pa.RunningStatus
	a.StartupTime = pa.StartupTime
	a.LatestHeartbeatTime = pa.LatestHeartbeatTime
	a.Tags = pa.Tags
	a.RunningDetails = pa.RunningDetails
}

func (a *Agent) AddRunningDetails(d *RunningStatistics) {
	if a.AgentId != d.AgentId {
		return
	}

	a.RunningDetails["Cpu"] = fmt.Sprintf("%f", d.Cpu)
	a.RunningDetails["Memory"] = strconv.FormatInt(d.Memory, 10)
	for k, v := range d.Extras {
		a.RunningDetails[k] = v
	}
}

type AgentAlarm struct {
	AlarmKey     string `json:"AlarmKey"`
	AlarmTime    string `json:"AlarmTime"`
	AlarmType    string `json:"AlarmType"`
	AlarmMessage string `json:"AlarmMessage"`
}

type RunningStatistics struct {
	AgentId string            `json:"AgentId"`
	Cpu     float32           `json:"Cpu"`
	Memory  int64             `json:"Memory"`
	Extras  map[string]string `json:"Extras"`
}

func (r *RunningStatistics) ToProto() *proto.RunningStatistics {
	pr := new(proto.RunningStatistics)
	pr.Cpu = r.Cpu
	pr.Memory = r.Memory
	pr.Extras = r.Extras
	return pr
}

func (r *RunningStatistics) ParseProto(pr *proto.RunningStatistics) {
	r.Cpu = pr.Cpu
	r.Memory = pr.Memory
	r.Extras = pr.Extras
}
