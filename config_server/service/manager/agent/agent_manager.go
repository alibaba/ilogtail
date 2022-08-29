package agentmanager

import (
	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
)

type AgentManager struct {
	AgentMessageList agentMessageList
}

const (
	opt_heartbeat string = "HEARTBEAT"
	opt_alarm     string = "ALARM"
	opt_status    string = "STATUS"
)

// batch write message from agent to databse
type agentMessageList struct {
	Alarm     map[string]*model.AgentAlarm
	Heartbeat map[string]*model.Machine
	Status    map[string]*model.AgentStatus
}

func (a *agentMessageList) Init() {
	a.Alarm = make(map[string]*model.AgentAlarm, 0)
	a.Heartbeat = make(map[string]*model.Machine, 0)
	a.Status = make(map[string]*model.AgentStatus, 0)
}

func (a *agentMessageList) Push(opt string, data interface{}) {
	common.Mutex().Lock("AgentMessageList")
	defer common.Mutex().Unlock("AgentMessageList")

	switch opt {
	case opt_alarm:
		k := data.(*model.AgentAlarm).MachineId
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
