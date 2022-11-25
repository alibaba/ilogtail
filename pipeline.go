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

package ilogtail

import (
	"sync"

	"github.com/alibaba/ilogtail/pkg/models"
)

type PipelineContext interface {
	Collector() PipelineCollector
}

type PipelineCollector interface {
	Collect(group *models.GroupInfo, events ...models.PipelineEvent)

	CollectAll(groups ...*models.PipelineGroupEvents)

	Dump() []*models.PipelineGroupEvents

	Observe() chan *models.PipelineGroupEvents
}

type observePipeCollector struct {
	groupChan chan *models.PipelineGroupEvents
}

func (p *observePipeCollector) Collect(group *models.GroupInfo, events ...models.PipelineEvent) {
	if len(events) == 0 {
		return
	}
	p.groupChan <- &models.PipelineGroupEvents{
		Group:  group,
		Events: events,
	}
}

func (p *observePipeCollector) CollectAll(groups ...*models.PipelineGroupEvents) {
	if len(groups) == 0 {
		return
	}
	for _, g := range groups {
		p.groupChan <- g
	}
}

func (p *observePipeCollector) Dump() []*models.PipelineGroupEvents {
	results := make([]*models.PipelineGroupEvents, len(p.groupChan))
	for i := 0; i < len(p.groupChan); i++ {
		results[i] = <-p.groupChan
	}
	return results
}

func (p *observePipeCollector) Observe() chan *models.PipelineGroupEvents {
	return p.groupChan
}

type groupedPipeCollector struct {
	groupEvents map[*models.GroupInfo][]models.PipelineEvent
}

func (p *groupedPipeCollector) Collect(group *models.GroupInfo, events ...models.PipelineEvent) {
	if len(events) == 0 {
		return
	}
	store, has := p.groupEvents[group]
	if !has {
		store = make([]models.PipelineEvent, 0)
	}
	p.groupEvents[group] = append(store, events...)
}

func (p *groupedPipeCollector) CollectAll(groups ...*models.PipelineGroupEvents) {
	if len(groups) == 0 {
		return
	}
	for _, g := range groups {
		p.Collect(g.Group, g.Events...)
	}
}

func (p *groupedPipeCollector) Dump() []*models.PipelineGroupEvents {
	len, idx := len(p.groupEvents), 0
	results := make([]*models.PipelineGroupEvents, len)
	if len == 0 {
		return results
	}
	for group, events := range p.groupEvents {
		results[idx] = &models.PipelineGroupEvents{
			Group:  group,
			Events: events,
		}
		idx++
	}
	p.groupEvents = make(map[*models.GroupInfo][]models.PipelineEvent)
	return results
}

func (p *groupedPipeCollector) Observe() chan *models.PipelineGroupEvents {
	return nil
}

type voidPipeCollector struct {
}

func (p *voidPipeCollector) Collect(group *models.GroupInfo, events ...models.PipelineEvent) {
}

func (p *voidPipeCollector) CollectAll(groups ...*models.PipelineGroupEvents) {
}

func (p *voidPipeCollector) Dump() []*models.PipelineGroupEvents {
	return nil
}

func (p *voidPipeCollector) Observe() chan *models.PipelineGroupEvents {
	return nil
}

type defaultPipelineContext struct {
	collector PipelineCollector
}

func (p *defaultPipelineContext) Collector() PipelineCollector {
	return p.collector
}

func NewObservePipelineConext(queueSize int) PipelineContext {
	return newPipelineConext(&observePipeCollector{
		groupChan: make(chan *models.PipelineGroupEvents, queueSize),
	})
}

func NewGroupedPipelineConext() PipelineContext {
	return newPipelineConext(&groupedPipeCollector{
		groupEvents: make(map[*models.GroupInfo][]models.PipelineEvent),
	})
}

func NewVoidPipelineConext() PipelineContext {
	return newPipelineConext(&voidPipeCollector{})
}

func newPipelineConext(collector PipelineCollector) PipelineContext {
	return &defaultPipelineContext{collector: collector}
}

type CancellationControl struct {
	cancelToken chan struct{}
	wg          sync.WaitGroup
}

func (p *CancellationControl) CancelToken() <-chan struct{} {
	return p.cancelToken
}

func (p *CancellationControl) Run(task func(*CancellationControl)) {
	p.wg.Add(1)
	go func(cc *CancellationControl, fn func(*CancellationControl)) {
		defer cc.wg.Done()
		fn(cc)
	}(p, task)
}

func (p *CancellationControl) Cancel() {
	close(p.cancelToken)
	p.wg.Wait()
}

func NewCancellationControl() *CancellationControl {
	return &CancellationControl{
		cancelToken: make(chan struct{}, 1),
	}
}
