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
	config, ok := c.ConfigList[configName]
	if ok {
		if config.DelTag == false { // exsit
			return true, nil
		} else { // exist but has delete tag
			config.Content = info
			config.Version = config.Version + 1
			config.Description = description
			config.DelTag = false

			c.ConfigList[configName] = config
			return false, nil
		}
	} else { // doesn't exist
		config := new(model.Config)
		config.Name = configName
		config.Content = info
		config.Version = 0
		config.Description = description
		config.DelTag = false

		c.ConfigList[configName] = config
		return false, nil
	}
}

func (c *ConfigManager) UpdateConfig(configName string, info string, description string) (bool, error) {
	config, ok := c.ConfigList[configName]
	if !ok {
		return false, nil
	} else {
		if config.DelTag == true {
			return false, nil
		} else {
			config.Content = info
			config.Version = config.Version + 1
			config.Description = description

			c.ConfigList[configName] = config
			return true, nil
		}
	}
}

func (c *ConfigManager) DeleteConfig(configName string) (bool, error) {
	config, ok := c.ConfigList[configName]
	if !ok {
		return false, nil
	} else {
		if config.DelTag == true {
			return false, nil
		} else {
			config.DelTag = true
			config.Version = config.Version + 1

			c.ConfigList[configName] = config
			return true, nil
		}
	}
}

func (c *ConfigManager) GetConfig(configName string) (*model.Config, error) {
	config, ok := c.ConfigList[configName]
	if !ok {
		return nil, nil
	} else {
		if config.DelTag == true {
			return nil, nil
		}
		return config, nil
	}
}

func (c *ConfigManager) ListAllConfigs() ([]model.Config, error) {
	configList := c.ConfigList
	ans := make([]model.Config, 0)
	for _, config := range configList {
		if config.DelTag == false {
			ans = append(ans, *config)
		}
	}
	return ans, nil
}

func (c *ConfigManager) UpdateConfigList() {
	ticker := time.NewTicker(3 * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		s := store.GetStore()
		b := store.CreateBacth()

		// push
		for k, v := range c.ConfigList {
			ok, err := s.Has(common.TYPE_COLLECTION_CONFIG, k)
			if err != nil {
				log.Println(err)
				continue
			} else if ok {
				value, err := s.Get(common.TYPE_COLLECTION_CONFIG, k)
				if err != nil {
					log.Println(err)
					continue
				}
				config := value.(*model.Config)

				if config.Version < v.Version {
					b.Update(common.TYPE_COLLECTION_CONFIG, k, v)
				}
			} else {
				b.Add(common.TYPE_COLLECTION_CONFIG, k, v)
			}
		}

		err := s.WriteBatch(b)
		if err != nil {
			log.Println(err)
			continue
		}

		// pull
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

func (c *ConfigManager) CheckConfigList(id string, configs map[string]string) ([]CheckResult, bool, bool, error) {
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
