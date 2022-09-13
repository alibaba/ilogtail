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
	"strconv"
	"sync"
	"time"

	"github.com/alibaba/ilogtail/config_server/service/common"
	"github.com/alibaba/ilogtail/config_server/service/model"
	proto "github.com/alibaba/ilogtail/config_server/service/proto"
	"github.com/alibaba/ilogtail/config_server/service/store"
)

func (a *AgentManager) HeartBeat(req *proto.HeartBeatRequest, res *proto.HeartBeatResponse) (int, *proto.HeartBeatResponse) {
	queryTime := time.Now().Unix()
	agent := new(model.Agent)
	agent.AgentId = req.AgentId
	agent.AgentType = req.AgentType
	agent.Version = req.AgentVersion
	agent.Ip = req.Ip
	agent.Tags = req.Tags
	agent.RunningStatus = req.RunningStatus
	agent.StartupTime = req.StartupTime
	agent.LatestHeartbeatTime = queryTime

	a.AgentMessageList.Push(opt_heartbeat, agent)

	res.Code = common.Accept.Code
	res.Message = "Send heartbeat success"
	return common.Accept.Status, res
}

func (a *AgentManager) RunningStatistics(req *proto.RunningStatisticsRequest, res *proto.RunningStatisticsResponse) (int, *proto.RunningStatisticsResponse) {
	agentStatus := new(model.RunningStatistics)
	agentStatus.ParseProto(req.RunningDetails)
	a.AgentMessageList.Push(opt_status, agentStatus)

	res.Code = common.Accept.Code
	res.Message = "Send running statistics success"
	return common.Accept.Status, res
}

func (a *AgentManager) Alarm(req *proto.AlarmRequest, res *proto.AlarmResponse) (int, *proto.AlarmResponse) {
	queryTime := strconv.FormatInt(time.Now().Unix(), 10)
	alarm := new(model.AgentAlarm)
	alarm.AlarmKey = generateAlarmKey(queryTime, req.AgentId)
	alarm.AlarmTime = queryTime
	alarm.AlarmType = req.Type
	alarm.AlarmMessage = req.Detail
	a.AgentMessageList.Push(opt_alarm, alarm)

	res.Code = common.Accept.Code
	res.Message = "Alarm success"
	return common.Accept.Status, res
}

var wg sync.WaitGroup

func (a *AgentManager) updateAgentMessage(interval int) {
	ticker := time.NewTicker(time.Duration(interval) * time.Second)
	defer ticker.Stop()

	for range ticker.C {
		wg.Add(3)
		go a.batchAddAlarm()
		go a.batchUpdateAgentMessage()
		go a.releaseAlarm()
		wg.Wait()
	}
}

func (a *AgentManager) batchAddAlarm() {
	s := store.GetStore()
	b := store.CreateBacth()

	a.AgentMessageList.Mutex.Lock()
	for k, v := range a.AgentMessageList.Alarm {
		b.Add(common.TYPE_AGENT_ALARM, k, v)
	}
	a.AgentMessageList.Alarm = make(map[string]*model.AgentAlarm, 0)
	a.AgentMessageList.Mutex.Unlock()

	err := s.WriteBatch(b)
	if err != nil {
		log.Println(err)
		return
	}

	wg.Done()
}

func (a *AgentManager) releaseAlarm() {
	s := store.GetStore()
	b := store.CreateBacth()

	alarmCount, err := s.Count(common.TYPE_AGENT_ALARM)
	if err != nil {
		log.Println(err)
		return
	}
	if alarmCount > 10000 {
		alarmList, err := s.GetAll(common.TYPE_AGENT_ALARM)
		if err != nil {
			log.Println(err)
			return
		}
		for i, v := range alarmList {
			if i > 5000 {
				break
			}
			b.Delete(common.TYPE_AGENT_ALARM, generateAlarmKey(v.(*model.AgentAlarm).AlarmTime, v.(*model.AgentAlarm).AlarmKey))
		}
	}

	err = s.WriteBatch(b)
	if err != nil {
		log.Println(err)
		return
	}

	wg.Done()
}

func (a *AgentManager) batchUpdateAgentMessage() {
	s := store.GetStore()
	b := store.CreateBacth()

	a.AgentMessageList.Mutex.Lock()
	for k, v := range a.AgentMessageList.Heartbeat {
		b.Update(common.TYPE_AGENT, k, v)
	}
	a.AgentMessageList.Heartbeat = make(map[string]*model.Agent, 0)
	for k, v := range a.AgentMessageList.Status {
		b.Update(common.TYPE_RUNNING_STATISTICS, k, v)
	}
	a.AgentMessageList.Status = make(map[string]*model.RunningStatistics, 0)
	a.AgentMessageList.Mutex.Unlock()

	err := s.WriteBatch(b)
	if err != nil {
		log.Println(err)
	}

	wg.Done()
}

func generateAlarmKey(queryTime string, agentId string) string {
	return queryTime + ":" + agentId
}
