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

package models

type PipelineEvent interface {
	GetName() string

	SetName(string)

	GetTags() Tags

	GetType() EventType

	GetTimestamp() uint64

	GetObservedTimestamp() uint64

	SetObservedTimestamp(uint64)
}

type GroupInfo struct {
	metadata Metadata
	tags     Tags
}

func (g *GroupInfo) Metadata() Metadata {
	if g != nil && g.metadata != nil {
		return g.metadata
	}
	return emptyStringValues
}

func (g *GroupInfo) Tags() Tags {
	if g != nil && g.tags != nil {
		return g.tags
	}
	return emptyStringValues
}

type PipelineGroupEvents struct {
	Group  *GroupInfo
	Events []PipelineEvent
}
