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
	"time"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	proto "github.com/alibaba/ilogtail/config_server/service/proto"
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

func (c *ConfigManager) GetConfigList(id string, configs map[string]int64) ([]*proto.ConfigUpdateInfo, bool, bool, error) {
	ans := make([]*proto.ConfigUpdateInfo, 0)
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
		match := func() bool {
			for _, v := range agentGroup.(*model.AgentGroup).Tags {
				_, ok := agent.Tags[v.Name]
				if ok && agent.Tags[v.Name] == v.Value {
					return true
				}
			}
			return false
		}()
		if match || agentGroup.(*model.AgentGroup).Name == "Default" {
			for k := range agentGroup.(*model.AgentGroup).AppliedConfigs {
				configList[k] = nil
			}
		}
	}

	// comp config info
	for k := range configs {
		if _, ok := configList[k]; !ok {
			result := new(proto.ConfigUpdateInfo)
			result.ConfigName = k
			result.UpdateStatus = proto.ConfigUpdateInfo_DELETED
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

		result := new(proto.ConfigUpdateInfo)

		if _, ok := configs[k]; !ok {
			result.ConfigName = k
			result.UpdateStatus = proto.ConfigUpdateInfo_NEW
			result.ConfigVersion = config.Version
			result.Content = config.Content
		} else {
			ver := configs[k]

			if ver < config.Version {
				result.ConfigName = k
				result.UpdateStatus = proto.ConfigUpdateInfo_MODIFIED
				result.ConfigVersion = config.Version
				result.Content = config.Content
			} else {
				result.ConfigName = k
				result.UpdateStatus = proto.ConfigUpdateInfo_SAME
			}
		}
		ans = append(ans, result)
	}

	return ans, true, true, nil
}
