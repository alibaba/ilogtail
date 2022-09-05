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

package model

type Agent struct {
	AgentId      string            `json:"AgentId"`
	Ip           string            `json:"Ip"`
	State        string            `json:"State"`
	Region       string            `json:"Region"`
	StartUpTime  int64             `json:"StartUpTime"`
	Env          string            `json:"Env"`
	Version      string            `json:"Version"`
	ConnectState string            `json:"ConnectState"`
	Heartbeat    string            `json:"Heartbeat"`
	Tag          map[string]string `json:"Tag"`
	Status       map[string]string `json:"Status"`
	Progress     map[string]string `json:"Progress"`
}

type AgentAlarm struct {
	AlarmKey     string `json:"AlarmKey"`
	AlarmTime    string `json:"AlarmTime"`
	AlarmType    string `json:"AlarmType"`
	AlarmMessage string `json:"AlarmMessage"`
}

type AgentStatus struct {
	AgentId  string            `json:"AgentId"`
	Status   map[string]string `json:"Status"`
	Progress map[string]string `json:"Progress"`
}
