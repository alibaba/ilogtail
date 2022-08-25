package configmanager

import (
	"strconv"
	"time"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

func (c *ConfigManager) CreateMachineGroup(groupName string, tag string, description string) (bool, error) {
	if tag == "" {
		tag = "default"
	}

	s := store.GetStore()
	ok, err := s.Has(common.TYPE_MACHINEGROUP, groupName)
	if err != nil {
		return true, err
	} else if ok {
		return true, nil
	} else {
		machineGroup := new(model.MachineGroup)
		machineGroup.Name = groupName
		machineGroup.Tag = tag
		machineGroup.Description = description
		machineGroup.AppliedConfigs = make(map[string]int64, 0)

		err = s.Add(common.TYPE_MACHINEGROUP, machineGroup.Name, machineGroup)
		if err != nil {
			return false, err
		}
		return false, nil
	}
}

func (c *ConfigManager) UpdateMachineGroup(groupName string, tag string, description string) (bool, error) {
	if tag == "" {
		tag = "default"
	}

	s := store.GetStore()
	ok, err := s.Has(common.TYPE_MACHINEGROUP, groupName)
	if err != nil {
		return false, err
	} else if !ok {
		return false, nil
	} else {
		value, err := s.Get(common.TYPE_MACHINEGROUP, groupName)
		if err != nil {
			return true, err
		}
		machineGroup := value.(*model.MachineGroup)

		machineGroup.Tag = tag
		machineGroup.Description = description
		machineGroup.Version++

		err = s.Update(common.TYPE_MACHINEGROUP, groupName, machineGroup)
		if err != nil {
			return true, err
		}
		return true, nil
	}
}

func (c *ConfigManager) DeleteMachineGroup(groupName string) (bool, error) {
	s := store.GetStore()
	ok, err := s.Has(common.TYPE_MACHINEGROUP, groupName)
	if err != nil {
		return false, err
	} else if !ok {
		return false, nil
	} else {
		err = s.Delete(common.TYPE_MACHINEGROUP, groupName)
		if err != nil {
			return true, err
		}
		return true, nil
	}
}

func (c *ConfigManager) GetMachineGroup(groupName string) (*model.MachineGroup, error) {
	s := store.GetStore()
	ok, err := s.Has(common.TYPE_MACHINEGROUP, groupName)
	if err != nil {
		return nil, err
	} else if !ok {
		return nil, nil
	} else {
		machineGroup, err := s.Get(common.TYPE_MACHINEGROUP, groupName)
		if err != nil {
			return nil, err
		}
		return machineGroup.(*model.MachineGroup), nil
	}
}

func (c *ConfigManager) GetAllMachineGroup() ([]model.MachineGroup, error) {
	s := store.GetStore()
	machineGroupList, err := s.GetAll(common.TYPE_MACHINEGROUP)
	if err != nil {
		return nil, err
	} else {
		ans := make([]model.MachineGroup, 0)
		for _, machineGroup := range machineGroupList {
			ans = append(ans, *machineGroup.(*model.MachineGroup))
		}
		return ans, nil
	}
}

func (a *ConfigManager) GetMachineList(groupName string) ([]model.Machine, error) {
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
				if status != nil {
					machine.Status = status.(*model.AgentStatus).Status
				}
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
	}
}

func (c *ConfigManager) ApplyConfigToMachineGroup(groupName string, configName string) (bool, bool, bool, error) {
	machineGroup, err := c.GetMachineGroup(groupName)
	if err != nil {
		return false, false, false, err
	}
	if machineGroup == nil {
		return false, false, false, nil
	}

	config, err := c.GetConfig(configName)
	if err != nil {
		return true, false, false, err
	}
	if config == nil {
		return true, false, false, nil
	}

	if _, ok := machineGroup.AppliedConfigs[config.Name]; ok {
		return true, true, true, nil
	}

	if machineGroup.AppliedConfigs == nil {
		machineGroup.AppliedConfigs = make(map[string]int64)
	}
	machineGroup.AppliedConfigs[config.Name] = time.Now().Unix()

	err = store.GetStore().Update(common.TYPE_MACHINEGROUP, groupName, machineGroup)
	if err != nil {
		return true, true, false, err
	}
	return true, true, false, nil
}

func (c *ConfigManager) RemoveConfigFromMachineGroup(groupName string, configName string) (bool, bool, bool, error) {
	machineGroup, err := c.GetMachineGroup(groupName)
	if err != nil {
		return false, false, false, err
	}
	if machineGroup == nil {
		return false, false, false, nil
	}

	config, err := c.GetConfig(configName)
	if err != nil {
		return true, false, false, err
	}
	if config == nil {
		return true, false, false, nil
	}

	if _, ok := machineGroup.AppliedConfigs[config.Name]; !ok {
		return true, true, false, nil
	}
	delete(machineGroup.AppliedConfigs, config.Name)

	err = store.GetStore().Update(common.TYPE_MACHINEGROUP, groupName, machineGroup)
	if err != nil {
		return true, true, true, err
	}
	return true, true, true, nil
}

func (c *ConfigManager) GetAppliedConfigs(groupName string) ([]string, bool, error) {
	ans := make([]string, 0)

	machineGroup, err := c.GetMachineGroup(groupName)
	if err != nil {
		return nil, false, err
	}
	if machineGroup == nil {
		return nil, false, nil
	}

	for k := range machineGroup.AppliedConfigs {
		ans = append(ans, k)
	}
	return ans, true, nil
}
