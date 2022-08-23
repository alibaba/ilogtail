package agentmanager

import (
	"log"
	"strconv"
	"time"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

const (
	opt_heartbeat string = "HEARTBEAT"
	opt_alarm     string = "ALARM"
	opt_status    string = "STATUS"
)

type agentMessage struct {
	Opt  string
	Data interface{}
}

func HeartBeat(id string, ip string, tags map[string]string) error {
	queryTime := strconv.FormatInt(time.Now().Unix(), 10)
	machine := new(model.Machine)
	machine.MachineId = id
	machine.Ip = ip
	machine.Heartbeat = queryTime
	machine.Tag = tags
	store.GetMemory().AgentMessageQueue.Push(agentMessage{opt_heartbeat, machine})
	return nil
}

func RunningStatus(id string, status map[string]string) error {
	machineStatus := new(model.AgentStatus)
	machineStatus.MachineId = id
	machineStatus.Status = status
	store.GetMemory().AgentMessageQueue.Push(agentMessage{opt_status, machineStatus})
	return nil
}

func updateAgentMessage() {
	ticker := time.NewTicker(1 * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		s := store.GetStore()
		b := store.CreateBacth()

		for !store.GetMemory().AgentMessageQueue.Empty() {
			msg := store.GetMemory().AgentMessageQueue.Pop().(agentMessage)
			switch msg.Opt {
			case opt_heartbeat:
				b.Update(common.TYPE_MACHINE, msg.Data.(*model.Machine).MachineId, msg.Data.(*model.Machine))
				break
			case opt_status:
				b.Update(common.TYPE_AGENT_STATUS, msg.Data.(*model.AgentStatus).MachineId, msg.Data.(*model.AgentStatus))
				break
			case opt_alarm:
				break
			}
		}

		err := s.WriteBatch(b)
		if err != nil {
			log.Println(err)
			continue
		}
	}
}

func init() {
	go updateAgentMessage()
}
