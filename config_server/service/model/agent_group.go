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

import proto "config-server/proto"

var Operators = map[string]proto.TagOperator{
	"LOGIC_AND": proto.TagOperator_LOGIC_AND,
	"LOGIC_OR":  proto.TagOperator_LOGIC_OR,
}

type AgentGroup struct {
	Name           string           `json:"Name"`
	Description    string           `json:"Description"`
	Tags           []AgentGroupTag  `json:"Tags"`
	TagOperator    string           `json:"TagOperator"`
	AppliedConfigs map[string]int64 `json:"AppliedConfigs"`
}

func (a *AgentGroup) ToProto() *proto.AgentGroup {
	pa := new(proto.AgentGroup)
	pa.GroupName = a.Name
	pa.Description = a.Description
	pa.Tags = make([]*proto.AgentGroupTag, 0)
	for _, v := range a.Tags {
		pa.Tags = append(pa.Tags, v.ToProto())
	}
	pa.TagOperator = Operators[a.TagOperator]
	return pa
}

func (a *AgentGroup) ParseProto(pa *proto.AgentGroup) {
	a.Name = pa.GroupName
	a.Description = pa.Description
	a.Tags = make([]AgentGroupTag, 0)
	for _, v := range pa.Tags {
		tag := new(AgentGroupTag)
		tag.ParseProto(v)
		a.Tags = append(a.Tags, *tag)
	}
	a.TagOperator = pa.TagOperator.String()
}
