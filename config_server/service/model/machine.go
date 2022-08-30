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

type Machine struct {
	MachineId string            `json:"instance_id"`
	Ip        string            `json:"ip"`
	State     string            `json:"state"`
	Heartbeat string            `json:"heartbeat"`
	Tag       map[string]string `json:"tags"`
	Status    map[string]string `json:"status"`
}

type AgentAlarm struct {
	AlarmKey     string `json:"alarm_key"`
	Time         string `json:"time"`
	AlarmType    string `json:"alarm_type"`
	AlarmMessage string `json:"alarm_message"`
}

type AgentStatus struct {
	MachineId string            `json:"instance_id"`
	Status    map[string]string `json:"status"`
}
