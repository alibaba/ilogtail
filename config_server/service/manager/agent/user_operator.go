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

package agentmanager

import (
	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

func (a *AgentManager) GetAgent(id string) (*model.Agent, error) {
	s := store.GetStore()
	ok, err := s.Has(common.TYPE_MACHINE, id)
	if err != nil {
		return nil, err
	} else if !ok {
		return nil, nil
	} else {
		agent, err := s.Get(common.TYPE_MACHINE, id)
		if err != nil {
			return nil, err
		}
		return agent.(*model.Agent), nil
	}
}
