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
	"log"
	"strconv"
	"time"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

func (c *ConfigManager) updateConfigList(interval int) {
	ticker := time.NewTicker(time.Duration(interval) * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		s := store.GetStore()
		configList, err := s.GetAll(common.TYPE_COLLECTION_CONFIG)
		if err != nil {
			log.Println(err)
			continue
		} else {
			c.ConfigListMutex.Lock()
			c.ConfigList = make(map[string]*model.Config, 0)
			for _, config := range configList {
				c.ConfigList[config.(*model.Config).Name] = config.(*model.Config)
			}
			c.ConfigListMutex.Unlock()
		}
	}
}

func (c *ConfigManager) PullConfigUpdates(id string, configs map[string]string) ([]CheckResult, bool, bool, error) {
	ans := make([]CheckResult, 0)
	s := store.GetStore()

	// get agent's tag
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
	agent := value.(*model.Agent)

	// get all configs connected to agent group whose tag is same as agent's
	configList := make(map[string]interface{})
	agentGroupList, err := s.GetAll(common.TYPE_MACHINEGROUP)
	if err != nil {
		return nil, false, true, err
	}

	for _, agentGroup := range agentGroupList {
		if _, ok := agent.Tag[agentGroup.(*model.AgentGroup).Tag]; ok || agentGroup.(*model.AgentGroup).Tag == "default" {
			for k := range agentGroup.(*model.AgentGroup).AppliedConfigs {
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
