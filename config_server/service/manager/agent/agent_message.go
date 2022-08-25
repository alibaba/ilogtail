package agentmanager

import (
	"log"
	"strconv"
	"time"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

func (a *AgentManager) HeartBeat(id string, ip string, tags map[string]string) error {
	queryTime := strconv.FormatInt(time.Now().Unix(), 10)
	machine := new(model.Machine)
	machine.MachineId = id
	machine.Ip = ip
	machine.Heartbeat = queryTime
	machine.Tag = tags
	a.AgentMessageList.Push(opt_heartbeat, machine)
	return nil
}

func (a *AgentManager) RunningStatus(id string, status map[string]string) error {
	machineStatus := new(model.AgentStatus)
	machineStatus.MachineId = id
	machineStatus.Status = status
	a.AgentMessageList.Push(opt_status, machineStatus)
	return nil
}

func (a *AgentManager) Alarm(id string, alarmType string, alarmMessage string) error {
	queryTime := strconv.FormatInt(time.Now().Unix(), 10)
	alarm := new(model.AgentAlarm)
	alarm.MachineId = queryTime + ":" + id
	alarm.Time = queryTime
	alarm.AlarmType = alarmType
	alarm.AlarmMessage = alarmMessage
	a.AgentMessageList.Push(opt_alarm, alarm)
	return nil
}

func (a *AgentManager) UpdateAgentMessage() {
	ticker := time.NewTicker(1 * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		s := store.GetStore()
		b := store.CreateBacth()

		for k, v := range a.AgentMessageList.Alarm {
			b.Update(common.TYPE_AGENT_ALARM, k, v)
		}
		for k, v := range a.AgentMessageList.Heartbeat {
			b.Update(common.TYPE_MACHINE, k, v)
		}
		for k, v := range a.AgentMessageList.Status {
			b.Update(common.TYPE_AGENT_STATUS, k, v)
		}

		alarmCount, err := s.Count(common.TYPE_AGENT_ALARM)
		if err != nil {
			log.Println(err)
			continue
		}
		if alarmCount > 10000 {
			alarmList, err := s.GetAll(common.TYPE_AGENT_ALARM)
			if err != nil {
				log.Println(err)
				continue
			}
			for i, v := range alarmList {
				if i > 5000 {
					break
				}
				b.Delete(common.TYPE_AGENT_ALARM, v.(*model.AgentAlarm).Time+":"+v.(*model.AgentAlarm).MachineId)
			}
		}

		err = s.WriteBatch(b)
		if err != nil {
			log.Println(err)
			continue
		}
		a.AgentMessageList.Init()
	}
}
