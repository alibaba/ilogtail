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

package manager

import (
	agentManager "github.com/alibaba/ilogtail/config_server/service/manager/agent"
	configManager "github.com/alibaba/ilogtail/config_server/service/manager/config"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/setting"
)

var myAgentManager *agentManager.AgentManager
var myConfigManager *configManager.ConfigManager

func AgentManager() *agentManager.AgentManager {
	return myAgentManager
}

func ConfigManager() *configManager.ConfigManager {
	return myConfigManager
}

func init() {
	myAgentManager = new(agentManager.AgentManager)
	myAgentManager.AgentMessageList.Init()
	go myAgentManager.UpdateAgentMessage(setting.GetSetting().AgentUpdateInterval)

	myConfigManager = new(configManager.ConfigManager)
	myConfigManager.ConfigList = make(map[string]*model.Config)
	go myConfigManager.UpdateConfigList(setting.GetSetting().ConfigSyncInterval)
}
