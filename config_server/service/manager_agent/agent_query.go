package agentmanager

import (
	"strconv"
	"time"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

func GetMachineList(groupName string) ([]model.Machine, error) {
	nowTime := time.Now()
	ans := make([]model.Machine, 0)
	s := store.GetStore()

	if groupName == "default" {
		machineList, err := s.GetAll(common.TYPE_MACHINE)
		if err != nil {
			return nil, err
		}

		for _, v := range machineList {
			machine := v.(*model.Machine)

			ok, err := s.Has(common.TYPE_AGENT_STATUS, machine.MachineId)
			if err != nil {
				return nil, err
			}
			if ok {
				status, err := s.Get(common.TYPE_AGENT_STATUS, machine.MachineId)
				if err != nil {
					return nil, err
				}
				machine.Status = status.(model.AgentStatus).Status
			} else {
				machine.Status = make(map[string]string, 0)
			}

			heartbeatTime, err := strconv.ParseInt(machine.Heartbeat, 10, 64)
			if err != nil {
				return nil, err
			}

			preHeart := nowTime.Sub(time.Unix(heartbeatTime, 0))
			if preHeart.Seconds() < 15 {
				machine.State = "good"
			} else if preHeart.Seconds() < 60 {
				machine.State = "bad"
			} else {
				machine.State = "lost"
			}

			ans = append(ans, *machine)
		}

		return ans, nil
	} else {
		return nil, nil
		/*
			s := store.GetStore()
			ok, err := s.Has(common.TYPE_MACHINEGROUP, groupName)
			if err != nil {
				return nil, err
			} else if !ok {
				return nil, nil
			} else {
				value, err := s.Get(common.TYPE_MACHINEGROUP, groupName)
				if err != nil {
					return nil, err
				}

				tag := value.(*model.MachineGroup).Tag

			}
		*/
	}
}
