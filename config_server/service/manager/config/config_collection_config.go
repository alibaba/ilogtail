package configmanager

import (
	"log"
	"strconv"
	"time"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

func (c *ConfigManager) CreateConfig(configName string, info string, description string) (bool, error) {
	common.Mutex().Lock(common.TYPE_COLLECTION_CONFIG)
	defer common.Mutex().Unlock(common.TYPE_COLLECTION_CONFIG)

	s := store.GetStore()
	ok, err := s.Has(common.TYPE_COLLECTION_CONFIG, configName)

	if err != nil {
		return false, err
	} else if ok {
		value, err := s.Get(common.TYPE_COLLECTION_CONFIG, configName)
		if err != nil {
			return true, err
		}
		config := value.(*model.Config)

		if config.DelTag == false { // exsit
			return true, nil
		} else { // exist but has delete tag
			config.Content = info
			config.Version = config.Version + 1
			config.Description = description
			config.DelTag = false

			err = s.Update(common.TYPE_COLLECTION_CONFIG, configName, config)
			if err != nil {
				return false, err
			}
			return false, nil
		}
	} else { // doesn't exist
		config := new(model.Config)
		config.Name = configName
		config.Content = info
		config.Version = 0
		config.Description = description
		config.DelTag = false

		err = s.Add(common.TYPE_COLLECTION_CONFIG, configName, config)
		if err != nil {
			return false, err
		}
		return false, nil
	}
}

func (c *ConfigManager) UpdateConfig(configName string, info string, description string) (bool, error) {
	common.Mutex().Lock(common.TYPE_COLLECTION_CONFIG)
	defer common.Mutex().Unlock(common.TYPE_COLLECTION_CONFIG)

	s := store.GetStore()
	ok, err := s.Has(common.TYPE_COLLECTION_CONFIG, configName)

	if err != nil {
		return false, err
	} else if !ok {
		return false, nil
	} else {
		value, err := s.Get(common.TYPE_COLLECTION_CONFIG, configName)
		if err != nil {
			return false, err
		}
		config := value.(*model.Config)

		if config.DelTag == true {
			return false, nil
		} else {
			config.Content = info
			config.Version = config.Version + 1
			config.Description = description

			err = s.Update(common.TYPE_COLLECTION_CONFIG, configName, config)
			if err != nil {
				return true, err
			}
			return true, nil
		}
	}
}

func (c *ConfigManager) DeleteConfig(configName string) (bool, error) {
	common.Mutex().Lock(common.TYPE_COLLECTION_CONFIG)
	defer common.Mutex().Unlock(common.TYPE_COLLECTION_CONFIG)

	s := store.GetStore()
	ok, err := s.Has(common.TYPE_COLLECTION_CONFIG, configName)

	if err != nil {
		return false, err
	} else if !ok {
		return false, nil
	} else {
		value, err := s.Get(common.TYPE_COLLECTION_CONFIG, configName)
		if err != nil {
			return false, err
		}
		config := value.(*model.Config)

		if config.DelTag == true {
			return false, nil
		} else {
			config.DelTag = true
			config.Version = config.Version + 1

			err = s.Update(common.TYPE_COLLECTION_CONFIG, configName, config)
			if err != nil {
				return true, err
			}
			return true, nil
		}
	}
}

func (c *ConfigManager) GetConfig(configName string) (*model.Config, error) {
	common.Mutex().RLock(common.TYPE_COLLECTION_CONFIG)
	defer common.Mutex().RUnlock(common.TYPE_COLLECTION_CONFIG)

	s := store.GetStore()
	ok, err := s.Has(common.TYPE_COLLECTION_CONFIG, configName)

	if err != nil {
		return nil, err
	} else if !ok {
		return nil, nil
	} else {
		value, err := s.Get(common.TYPE_COLLECTION_CONFIG, configName)
		if err != nil {
			return nil, err
		}
		config := value.(*model.Config)

		if config.DelTag == true {
			return nil, nil
		}
		return config, nil
	}
}

func (c *ConfigManager) ListAllConfigs() ([]model.Config, error) {
	common.Mutex().RLock(common.TYPE_COLLECTION_CONFIG)
	defer common.Mutex().RUnlock(common.TYPE_COLLECTION_CONFIG)

	s := store.GetStore()
	configs, err := s.GetAll(common.TYPE_COLLECTION_CONFIG)

	if err != nil {
		return nil, err
	} else {
		ans := make([]model.Config, 0)
		for _, value := range configs {
			config := value.(*model.Config)
			if config.DelTag == false {
				ans = append(ans, *config)
			}
		}
		return ans, nil
	}
}

func (c *ConfigManager) GetAppliedMachineGroups(configName string) ([]string, bool, error) {
	ans := make([]string, 0)

	config, err := c.GetConfig(configName)
	if err != nil {
		return nil, false, err
	}
	if config == nil {
		return nil, false, nil
	}

	machineGroupList, err := c.GetAllMachineGroup()
	if err != nil {
		return nil, true, err
	}

	for _, g := range machineGroupList {
		if _, ok := g.AppliedConfigs[configName]; ok {
			ans = append(ans, g.Name)
		}
	}
	return ans, true, nil
}

func (c *ConfigManager) UpdateConfigList(interval int) {
	ticker := time.NewTicker(time.Duration(interval) * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		common.Mutex().RLock(common.TYPE_COLLECTION_CONFIG)
		defer common.Mutex().RUnlock(common.TYPE_COLLECTION_CONFIG)

		s := store.GetStore()
		configList, err := s.GetAll(common.TYPE_COLLECTION_CONFIG)
		if err != nil {
			log.Println(err)
			continue
		} else {
			for k := range c.ConfigList {
				delete(c.ConfigList, k)
			}

			for _, config := range configList {
				c.ConfigList[config.(*model.Config).Name] = config.(*model.Config)
			}
		}
	}
}

func (c *ConfigManager) GetConfigUpdates(id string, configs map[string]string) ([]CheckResult, bool, bool, error) {
	common.Mutex().RLock(common.TYPE_MACHINE)
	defer common.Mutex().RUnlock(common.TYPE_MACHINE)
	common.Mutex().RLock(common.TYPE_MACHINEGROUP)
	defer common.Mutex().RUnlock(common.TYPE_MACHINEGROUP)

	ans := make([]CheckResult, 0)
	s := store.GetStore()

	// get machine's tag
	ok, err := s.Has(common.TYPE_MACHINE, id)
	if err != nil {
		return nil, false, false, err
	} else if !ok {
		return nil, false, false, nil
	}

	value, err := s.Get(common.TYPE_MACHINE, id)
	if err != nil {
		return nil, false, false, err
	}
	machine := value.(*model.Machine)

	// get all configs connected to machine group whose tag is same as machine's
	configList := make(map[string]interface{})
	machineGroupList, err := s.GetAll(common.TYPE_MACHINEGROUP)
	if err != nil {
		return nil, false, true, err
	}

	for _, machineGroup := range machineGroupList {
		if _, ok := machine.Tag[machineGroup.(*model.MachineGroup).Tag]; ok || machineGroup.(*model.MachineGroup).Tag == "default" {
			for k := range machineGroup.(*model.MachineGroup).AppliedConfigs {
				configList[k] = nil
			}
		}
	}

	// comp config info
	for k := range configs {
		if _, ok := configList[k]; !ok {
			var result CheckResult
			result.ConfigName = k
			result.Msg = "delete"
			ans = append(ans, result)
		}
	}

	for k := range configList {
		config, err := c.GetConfig(k)
		if err != nil {
			return nil, false, true, err
		}
		if config == nil {
			return nil, false, true, nil
		}

		var result CheckResult

		if _, ok := configs[k]; !ok {
			result.ConfigName = k
			result.Msg = "create"
			result.Data = *config
		} else {
			ver, err := strconv.Atoi(configs[k])
			if err != nil {
				return nil, true, true, err
			}

			if ver < config.Version {
				result.ConfigName = k
				result.Msg = "change"
				result.Data = *config
			} else {
				result.ConfigName = k
				result.Msg = "same"
			}
		}
		ans = append(ans, result)
	}

	return ans, true, true, nil
}
