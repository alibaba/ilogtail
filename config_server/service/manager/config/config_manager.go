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
	"sync"

	"config-server/common"
	"config-server/model"
	"config-server/setting"
	"config-server/store"
)

type ConfigManager struct {
	ConfigList      map[string]*model.ConfigDetail
	ConfigListMutex *sync.RWMutex
}

func (c *ConfigManager) Init() {
	c.ConfigList = make(map[string]*model.ConfigDetail)
	c.ConfigListMutex = new(sync.RWMutex)
	go c.updateConfigList(setting.GetSetting().ConfigSyncInterval)

	s := store.GetStore()
	ok, hasErr := s.Has(common.TypeAgentGROUP, "default")
	if hasErr != nil {
		panic(hasErr)
	}
	if ok {
		return
	}
	agentGroup := new(model.AgentGroup)
	agentGroup.Name = "default"
	agentGroup.Tags = make([]model.AgentGroupTag, 0)
	agentGroup.AppliedConfigs = map[string]int64{}
	agentGroup.Description = "Default agent group, include all Agents."

	addErr := s.Add(common.TypeAgentGROUP, agentGroup.Name, agentGroup)
	if addErr != nil {
		panic(addErr)
	}
}

func (c *ConfigManager) tagsMatch(groupTags []model.AgentGroupTag, agentTags []model.AgentGroupTag, tagOperator string) bool {
	switch tagOperator {
	case "LOGIC_AND":
		return logicAndMatch(groupTags, agentTags)
	case "LOGIC_OR":
		return logicOrMatch(groupTags, agentTags)
	default:
		return false
	}
}

func logicAndMatch(groupTags []model.AgentGroupTag, agentTags []model.AgentGroupTag) bool {
	for _, gt := range groupTags {
		found := false
		for _, at := range agentTags {
			if gt.Name == at.Name && gt.Value == at.Value {
				found = true
				break
			}
		}
		if !found {
			return false
		}
	}
	return true
}

func logicOrMatch(groupTags []model.AgentGroupTag, agentTags []model.AgentGroupTag) bool {
	for _, gt := range groupTags {
		for _, at := range agentTags {
			if gt.Name == at.Name && gt.Value == at.Value {
				return true
			}
		}
	}
	return false
}
