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

package configmanager

import (
	"time"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	proto "github.com/alibaba/ilogtail/config_server/service/proto"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

func (c *ConfigManager) CreateConfig(pconfig *proto.Config) (bool, error) {
	s := store.GetStore()
	ok, err := s.Has(common.TYPE_COLLECTION_CONFIG, pconfig.ConfigName)

	if err != nil {
		return false, err
	} else if ok {
		value, err := s.Get(common.TYPE_COLLECTION_CONFIG, pconfig.ConfigName)
		if err != nil {
			return true, err
		}
		config := value.(*model.Config)

		if !config.DelTag { // exsit
			return true, nil
		} else { // exist but has delete tag
			config.ParseProto(pconfig)
			config.Version = config.Version + 1
			config.DelTag = false

			err = s.Update(common.TYPE_COLLECTION_CONFIG, config.Name, config)
			if err != nil {
				return false, err
			}
			return false, nil
		}
	} else { // doesn't exist
		config := new(model.Config)
		config.ParseProto(pconfig)
		config.Version = 0
		config.DelTag = false

		err = s.Add(common.TYPE_COLLECTION_CONFIG, config.Name, config)
		if err != nil {
			return false, err
		}
		return false, nil
	}
}

func (c *ConfigManager) UpdateConfig(pconfig *proto.Config) (bool, error) {
	s := store.GetStore()
	ok, err := s.Has(common.TYPE_COLLECTION_CONFIG, pconfig.ConfigName)

	if err != nil {
		return false, err
	} else if !ok {
		return false, nil
	} else {
		value, err := s.Get(common.TYPE_COLLECTION_CONFIG, pconfig.ConfigName)
		if err != nil {
			return false, err
		}
		config := value.(*model.Config)

		if !config.DelTag {
			return false, nil
		} else {
			config.ParseProto(pconfig)
			config.Version = config.Version + 1

			err = s.Update(common.TYPE_COLLECTION_CONFIG, config.Name, config)
			if err != nil {
				return true, err
			}
			return true, nil
		}
	}
}

func (c *ConfigManager) DeleteConfig(configName string) (bool, error) {
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

		if !config.DelTag {
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

		if !config.DelTag {
			return nil, nil
		}
		return config, nil
	}
}

func (c *ConfigManager) ListAllConfigs() ([]model.Config, error) {
	s := store.GetStore()
	configs, err := s.GetAll(common.TYPE_COLLECTION_CONFIG)

	if err != nil {
		return nil, err
	} else {
		ans := make([]model.Config, 0)
		for _, value := range configs {
			config := value.(*model.Config)
			if !config.DelTag {
				ans = append(ans, *config)
			}
		}
		return ans, nil
	}
}

func (c *ConfigManager) GetAppliedAgentGroups(configName string) ([]string, bool, error) {
	ans := make([]string, 0)

	config, err := c.GetConfig(configName)
	if err != nil {
		return nil, false, err
	}
	if config == nil {
		return nil, false, nil
	}

	agentGroupList, err := c.GetAllAgentGroup()
	if err != nil {
		return nil, true, err
	}

	for _, g := range agentGroupList {
		if _, ok := g.AppliedConfigs[configName]; ok {
			ans = append(ans, g.Name)
		}
	}
	return ans, true, nil
}

func (c *ConfigManager) CreateAgentGroup(group *proto.AgentGroup) (bool, error) {
	s := store.GetStore()
	ok, err := s.Has(common.TYPE_AGENTGROUP, group.GroupName)
	if err != nil {
		return true, err
	} else if ok {
		return true, nil
	} else {
		agentGroup := new(model.AgentGroup)
		agentGroup.ParseProto(group)
		agentGroup.AppliedConfigs = make(map[string]int64, 0)

		err = s.Add(common.TYPE_AGENTGROUP, agentGroup.Name, agentGroup)
		if err != nil {
			return false, err
		}
		return false, nil
	}
}

func (c *ConfigManager) UpdateAgentGroup(group *proto.AgentGroup) (bool, error) {
	s := store.GetStore()
	ok, err := s.Has(common.TYPE_AGENTGROUP, group.GroupName)
	if err != nil {
		return false, err
	} else if !ok {
		return false, nil
	} else {
		value, err := s.Get(common.TYPE_AGENTGROUP, group.GroupName)
		if err != nil {
			return true, err
		}
		agentGroup := value.(*model.AgentGroup)

		agentGroup.ParseProto(group)

		err = s.Update(common.TYPE_AGENTGROUP, group.GroupName, agentGroup)
		if err != nil {
			return true, err
		}
		return true, nil
	}
}

func (c *ConfigManager) DeleteAgentGroup(groupName string) (bool, error) {
	s := store.GetStore()
	ok, err := s.Has(common.TYPE_AGENTGROUP, groupName)
	if err != nil {
		return false, err
	} else if !ok {
		return false, nil
	} else {
		err = s.Delete(common.TYPE_AGENTGROUP, groupName)
		if err != nil {
			return true, err
		}
		return true, nil
	}
}

func (c *ConfigManager) GetAgentGroup(groupName string) (*model.AgentGroup, error) {
	s := store.GetStore()
	ok, err := s.Has(common.TYPE_AGENTGROUP, groupName)
	if err != nil {
		return nil, err
	} else if !ok {
		return nil, nil
	} else {
		agentGroup, err := s.Get(common.TYPE_AGENTGROUP, groupName)
		if err != nil {
			return nil, err
		}
		return agentGroup.(*model.AgentGroup), nil
	}
}

func (c *ConfigManager) GetAllAgentGroup() ([]model.AgentGroup, error) {
	s := store.GetStore()
	agentGroupList, err := s.GetAll(common.TYPE_AGENTGROUP)
	if err != nil {
		return nil, err
	} else {
		ans := make([]model.AgentGroup, 0)
		for _, agentGroup := range agentGroupList {
			ans = append(ans, *agentGroup.(*model.AgentGroup))
		}
		return ans, nil
	}
}

func (a *ConfigManager) GetAgentList(groupName string) ([]model.Agent, error) {
	ans := make([]model.Agent, 0)
	s := store.GetStore()

	if groupName == "default" {
		agentList, err := s.GetAll(common.TYPE_AGENT)
		if err != nil {
			return nil, err
		}

		for _, v := range agentList {
			agent := v.(*model.Agent)

			ok, err := s.Has(common.TYPE_RUNNING_STATISTICS, agent.AgentId)
			if err != nil {
				return nil, err
			}
			if ok {
				status, err := s.Get(common.TYPE_RUNNING_STATISTICS, agent.AgentId)
				if err != nil {
					return nil, err
				}
				if status != nil {
					agent.AddRunningDetails(status.(*model.RunningStatistics))
				}
			} else {
				agent.RunningDetails = make(map[string]string, 0)
			}

			ans = append(ans, *agent)
		}

		return ans, nil
	} else {
		return nil, nil
	}
}

func (c *ConfigManager) ApplyConfigToAgentGroup(groupName string, configName string) (bool, bool, bool, error) {
	agentGroup, err := c.GetAgentGroup(groupName)
	if err != nil {
		return false, false, false, err
	}
	if agentGroup == nil {
		return false, false, false, nil
	}

	config, err := c.GetConfig(configName)
	if err != nil {
		return true, false, false, err
	}
	if config == nil {
		return true, false, false, nil
	}

	if _, ok := agentGroup.AppliedConfigs[config.Name]; ok {
		return true, true, true, nil
	}

	if agentGroup.AppliedConfigs == nil {
		agentGroup.AppliedConfigs = make(map[string]int64)
	}
	agentGroup.AppliedConfigs[config.Name] = time.Now().Unix()

	err = store.GetStore().Update(common.TYPE_AGENTGROUP, groupName, agentGroup)
	if err != nil {
		return true, true, false, err
	}
	return true, true, false, nil
}

func (c *ConfigManager) RemoveConfigFromAgentGroup(groupName string, configName string) (bool, bool, bool, error) {
	agentGroup, err := c.GetAgentGroup(groupName)
	if err != nil {
		return false, false, false, err
	}
	if agentGroup == nil {
		return false, false, false, nil
	}

	config, err := c.GetConfig(configName)
	if err != nil {
		return true, false, false, err
	}
	if config == nil {
		return true, false, false, nil
	}

	if _, ok := agentGroup.AppliedConfigs[config.Name]; !ok {
		return true, true, false, nil
	}
	delete(agentGroup.AppliedConfigs, config.Name)

	err = store.GetStore().Update(common.TYPE_AGENTGROUP, groupName, agentGroup)
	if err != nil {
		return true, true, true, err
	}
	return true, true, true, nil
}

func (c *ConfigManager) GetAppliedConfigsForAgentGroup(groupName string) ([]string, bool, error) {
	ans := make([]string, 0)

	agentGroup, err := c.GetAgentGroup(groupName)
	if err != nil {
		return nil, false, err
	}
	if agentGroup == nil {
		return nil, false, nil
	}

	for k := range agentGroup.AppliedConfigs {
		ans = append(ans, k)
	}
	return ans, true, nil
}
