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

// Shuffler allow routing data to other ilogtail instance and receive data from other instance.
// It should also implement Extension interface.
type Shuffler interface {
	// CollectList collect GroupEvents list that have been grouped, it sends the GroupEvents to the send channel.
	// It blocks if channel is full.
	CollectList(groupEventsList ...*models.PipelineGroupEvents)

	// Plugins can send data to shuffler with this channel
	SendChan() chan<- *models.PipelineGroupEvents

	// Plugins can fetch data from shuffler with this channel.
	RecvChan() <-chan *models.PipelineGroupEvents

	// Plugins can be notified from this channel if shuffler is stopped.
	Done() <-chan struct{}
}
