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
	"log"
	"sync"
	"time"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	proto "github.com/alibaba/ilogtail/config_server/service/proto"
	"github.com/alibaba/ilogtail/config_server/service/setting"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

const (
	STATUS_INIT    string = "INIT"
	STATUS_ONLINE  string = "ONLINE"
	STATUS_OFFLINE string = "OFFLINE"
)

func (a *AgentManager) HeartBeat(req *proto.HeartBeatRequest, res *proto.HeartBeatResponse) (int, *proto.HeartBeatResponse) {
	agent := new(model.Agent)
	agent.AgentID = req.AgentId
	agent.AgentType = req.AgentType
	agent.Attributes.ParseProto(req.Attributes)
	agent.Tags = req.Tags
	agent.Interval = req.Interval
	current := time.Now()
	startupTime := current.Unix()
	runningStatus := STATUS_INIT
	var successBeatCount int32 = 1
	s := store.GetStore()
	value, err := s.Get(common.TypeAgent, req.AgentId)
	if err == nil {
		prev := value.(*model.Agent)
		startupTime = prev.StartupTime
		runningStatus = prev.RunningStatus
		successBeatCount = prev.SuccessBeatCount + 1
	}
	onlineAfterSuccess := int32(setting.GetSetting().OnlineAfterSuccess)
	if successBeatCount >= onlineAfterSuccess {
		runningStatus = STATUS_ONLINE
		successBeatCount = onlineAfterSuccess
	}
	agent.StartupTime = startupTime
	agent.LatestBeatTime = current.Unix()
	agent.BeatCycleTime = current.Unix()
	agent.RunningStatus = runningStatus
	agent.SuccessBeatCount = successBeatCount
	agent.FailBeatCount = 0

	a.AgentMessageList.Push(optHeartbeat, agent)

	res.Code = proto.RespCode_ACCEPT
	res.Message = "Send heartbeat success"
	return common.Accept.Status, res
}

var wg sync.WaitGroup

func (a *AgentManager) updateAgentMessage(interval int) {
	ticker := time.NewTicker(time.Duration(interval) * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		wg.Add(1)
		go a.batchUpdateAgentMessage()
		wg.Wait()
	}
}

func (a *AgentManager) batchUpdateAgentMessage() {
	s := store.GetStore()
	b := store.CreateBacth()

	a.AgentMessageList.Mutex.Lock()
	for k, v := range a.AgentMessageList.Heartbeat {
		b.Update(common.TypeAgent, k, v)
	}
	a.AgentMessageList.Heartbeat = make(map[string]*model.Agent, 0)
	a.AgentMessageList.Mutex.Unlock()

	writeBatchErr := s.WriteBatch(b)
	if writeBatchErr != nil {
		log.Println(writeBatchErr)
	}

	a.handleFailure()

	wg.Done()
}

func (a *AgentManager) handleFailure() {
	s := store.GetStore()
	agentList, err := s.GetAll(common.TypeAgent)
	if err != nil {
		return
	}

	current := time.Now()
	deadline := current.Add(-time.Duration(setting.GetSetting().ClearHoursAfterOffline) * time.Hour).Unix()
	for _, v := range agentList {
		agent := v.(*model.Agent)
		if agent.LatestBeatTime < deadline {
			s.Delete(common.TypeAgent, agent.AgentID)
			continue
		}
		if agent.Interval == 0 {
			continue
		}
		duration := current.Unix() - int64(setting.GetSetting().AgentUpdateInterval) - agent.BeatCycleTime
		isFresh := duration/int64(agent.Interval) == 0
		if isFresh {
			continue
		}

		runningStatus := agent.RunningStatus
		failBeatCount := agent.FailBeatCount + 1
		offlineAfterFail := int32(setting.GetSetting().OfflineAfterFail)
		if failBeatCount >= offlineAfterFail {
			runningStatus = STATUS_OFFLINE
			failBeatCount = offlineAfterFail
		}
		agent.BeatCycleTime = current.Unix()
		agent.RunningStatus = runningStatus
		agent.SuccessBeatCount = 0
		agent.FailBeatCount = failBeatCount
		s.Update(common.TypeAgent, agent.AgentID, agent)
	}
}
