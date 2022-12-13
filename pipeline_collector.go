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
	"github.com/alibaba/ilogtail/pkg/models"
)

// PipelineCollector. Collect data in the plugin and send the data to the next operator
type PipelineCollector interface {

	// Collect single group and events data belonging to this group
	Collect(groupInfo *models.GroupInfo, eventList ...models.PipelineEvent)

	// CollectList collect GroupEvents list that have been grouped
<<<<<<< HEAD
	CollectList(groupEventsList ...*models.GroupedEvents)

	// ToArray returns an array containing all of the PipelineGroupEvents in this collector.
	ToArray() []*models.GroupedEvents

	// Observe returns a chan that can consume PipelineGroupEvents from this collector.
	Observe() chan *models.GroupedEvents
=======
	CollectList(groupEventsList ...*models.PipelineGroupEvents)

	// ToArray returns an array containing all of the PipelineGroupEvents in this collector.
	ToArray() []*models.PipelineGroupEvents

	// Observe returns a chan that can consume PipelineGroupEvents from this collector.
	Observe() chan *models.PipelineGroupEvents
>>>>>>> main

	Close()
}

// Observable pipeline data collector, which stores data based on channal and can be subscribed by multiple consumers
type observePipeCollector struct {
<<<<<<< HEAD
	groupChan chan *models.GroupedEvents
=======
	groupChan chan *models.PipelineGroupEvents
>>>>>>> main
}

func (p *observePipeCollector) Collect(group *models.GroupInfo, events ...models.PipelineEvent) {
	if len(events) == 0 {
		return
	}
<<<<<<< HEAD
	p.groupChan <- &models.GroupedEvents{
=======
	p.groupChan <- &models.PipelineGroupEvents{
>>>>>>> main
		Group:  group,
		Events: events,
	}
}

<<<<<<< HEAD
func (p *observePipeCollector) CollectList(groups ...*models.GroupedEvents) {
=======
func (p *observePipeCollector) CollectList(groups ...*models.PipelineGroupEvents) {
>>>>>>> main
	if len(groups) == 0 {
		return
	}
	for _, g := range groups {
		p.groupChan <- g
	}
}

<<<<<<< HEAD
func (p *observePipeCollector) ToArray() []*models.GroupedEvents {
	results := make([]*models.GroupedEvents, len(p.groupChan))
=======
func (p *observePipeCollector) ToArray() []*models.PipelineGroupEvents {
	results := make([]*models.PipelineGroupEvents, len(p.groupChan))
>>>>>>> main
	for i := 0; i < len(p.groupChan); i++ {
		results[i] = <-p.groupChan
	}
	return results
}

<<<<<<< HEAD
func (p *observePipeCollector) Observe() chan *models.GroupedEvents {
=======
func (p *observePipeCollector) Observe() chan *models.PipelineGroupEvents {
>>>>>>> main
	return p.groupChan
}

func (p *observePipeCollector) Close() {
	close(p.groupChan)
}

// groupedPipeCollector group the collected PipelineEvent by groupInfo.
// The limitation is that it cannot be subscribed as it always returns an empty chan.
// so it can only return all the grouped data at one time.
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

<<<<<<< HEAD
func (p *groupedPipeCollector) CollectList(groups ...*models.GroupedEvents) {
=======
func (p *groupedPipeCollector) CollectList(groups ...*models.PipelineGroupEvents) {
>>>>>>> main
	if len(groups) == 0 {
		return
	}
	for _, g := range groups {
		p.Collect(g.Group, g.Events...)
	}
}

<<<<<<< HEAD
func (p *groupedPipeCollector) ToArray() []*models.GroupedEvents {
	len, idx := len(p.groupEvents), 0
	results := make([]*models.GroupedEvents, len)
=======
func (p *groupedPipeCollector) ToArray() []*models.PipelineGroupEvents {
	len, idx := len(p.groupEvents), 0
	results := make([]*models.PipelineGroupEvents, len)
>>>>>>> main
	if len == 0 {
		return results
	}
	for group, events := range p.groupEvents {
<<<<<<< HEAD
		results[idx] = &models.GroupedEvents{
=======
		results[idx] = &models.PipelineGroupEvents{
>>>>>>> main
			Group:  group,
			Events: events,
		}
		idx++
	}
	p.groupEvents = make(map[*models.GroupInfo][]models.PipelineEvent)
	return results
}

<<<<<<< HEAD
func (p *groupedPipeCollector) Observe() chan *models.GroupedEvents {
=======
func (p *groupedPipeCollector) Observe() chan *models.PipelineGroupEvents {
>>>>>>> main
	return nil
}

func (p *groupedPipeCollector) Close() {
	for k := range p.groupEvents {
		delete(p.groupEvents, k)
	}
}

// noopPipeCollector is an empty collector implementation.
type noopPipeCollector struct {
}

func (p *noopPipeCollector) Collect(group *models.GroupInfo, events ...models.PipelineEvent) {
}

<<<<<<< HEAD
func (p *noopPipeCollector) CollectList(groups ...*models.GroupedEvents) {
}

func (p *noopPipeCollector) ToArray() []*models.GroupedEvents {
	return nil
}

func (p *noopPipeCollector) Observe() chan *models.GroupedEvents {
=======
func (p *noopPipeCollector) CollectList(groups ...*models.PipelineGroupEvents) {
}

func (p *noopPipeCollector) ToArray() []*models.PipelineGroupEvents {
	return nil
}

func (p *noopPipeCollector) Observe() chan *models.PipelineGroupEvents {
>>>>>>> main
	return nil
}

func (p *noopPipeCollector) Close() {
}

type defaultPipelineContext struct {
	collector PipelineCollector
}

func (p *defaultPipelineContext) Collector() PipelineCollector {
	return p.collector
}

func NewObservePipelineConext(queueSize int) PipelineContext {
	return newPipelineConext(&observePipeCollector{
<<<<<<< HEAD
		groupChan: make(chan *models.GroupedEvents, queueSize),
=======
		groupChan: make(chan *models.PipelineGroupEvents, queueSize),
>>>>>>> main
	})
}

func NewGroupedPipelineConext() PipelineContext {
	return newPipelineConext(&groupedPipeCollector{
		groupEvents: make(map[*models.GroupInfo][]models.PipelineEvent),
	})
}

func NewNoopPipelineConext() PipelineContext {
	return newPipelineConext(&noopPipeCollector{})
}

func newPipelineConext(collector PipelineCollector) PipelineContext {
	return &defaultPipelineContext{collector: collector}
}
