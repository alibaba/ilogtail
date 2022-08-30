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

package agentmanager

import (
	"sync"

	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/setting"
)

type AgentManager struct {
	AgentMessageList agentMessageList
}

func (a *AgentManager) Init() {
	a.AgentMessageList.Clear()
	go a.updateAgentMessage(setting.GetSetting().AgentUpdateInterval)
}

// batch write message from agent to databse

const (
	opt_heartbeat string = "HEARTBEAT"
	opt_alarm     string = "ALARM"
	opt_status    string = "STATUS"
)

type agentMessageList struct {
	Alarm     map[string]*model.AgentAlarm
	Heartbeat map[string]*model.Machine
	Status    map[string]*model.AgentStatus
	Mutex     sync.RWMutex
}

func (a *agentMessageList) Clear() {
	a.Alarm = make(map[string]*model.AgentAlarm, 0)
	a.Heartbeat = make(map[string]*model.Machine, 0)
	a.Status = make(map[string]*model.AgentStatus, 0)
}

func (a *agentMessageList) Push(opt string, data interface{}) {
	a.Mutex.Lock()
	defer a.Mutex.Unlock()

	switch opt {
	case opt_alarm:
		k := data.(*model.AgentAlarm).AlarmKey
		v := data.(*model.AgentAlarm)
		a.Alarm[k] = v
		break
	case opt_heartbeat:
		k := data.(*model.Machine).MachineId
		v := data.(*model.Machine)
		a.Heartbeat[k] = v
		break
	case opt_status:
		k := data.(*model.AgentStatus).MachineId
		v := data.(*model.AgentStatus)
		a.Status[k] = v
		break
	}
}
