// Copyright 2023 iLogtail Authors
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

package extensions

import (
	"github.com/alibaba/ilogtail/pkg/models"
)

// Shuffler allow routing data to other ilogtail instance and receive data from other instance
type Shuffler interface {
	Start() error
	Stop() error

	// CollectList collect GroupEvents list that have been grouped, it sends the GroupEvents to the send channel.
	// If it blocks if channel is full.
	CollectList(groupEventsList ...*models.PipelineGroupEvents)

	SendChan() chan<- *models.PipelineGroupEvents
	RecvChan() <-chan *models.PipelineGroupEvents
	Done() <-chan struct{}
}
