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

package metadatagroup

import (
	"fmt"
	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/models"
	"strings"
	"sync"
)

const (
	connector       = "_"
	MaxEventsLength = 1024
	MaxBytesLength  = 2 * 1024 * 1024
)

type metadataGroup struct {
	//defaultPipeGroupEvents *models.PipelineGroupEvents
	Group  *models.GroupInfo
	Events []models.PipelineEvent

	nowEventsLength     int
	nowEventsByteLength int
	MaxEventsLength     int
	MaxEventsByteLength int

	Lock *sync.Mutex
}

func (g *metadataGroup) Record(group *models.PipelineGroupEvents, ctx ilogtail.PipelineContext) error {
	g.Lock.Lock()
	defer g.Lock.Unlock()
	eventsLen := len(group.Events)
	bytesLen := g.evaluateByteLength(group.Events)

	//When current events is full, make a quick flush.
	if g.nowEventsLength+eventsLen > g.MaxEventsLength || g.nowEventsByteLength+bytesLen > g.MaxEventsByteLength {
		ctx.Collector().Collect(g.Group, g.Events...)

		//reset length
		g.Reset()
	}

	//add events
	g.nowEventsLength += eventsLen
	g.nowEventsByteLength += bytesLen
	g.Events = append(g.Events, group.Events...)
	return nil
}

func (g *metadataGroup) GetResult(ctx ilogtail.PipelineContext) error {
	g.Lock.Lock()
	if len(g.Events) == 0 {
		g.Lock.Unlock()
		return nil
	}

	eventsToFlush := g.Events
	//reset
	g.Reset()
	g.Lock.Unlock()

	ctx.Collector().Collect(g.Group, eventsToFlush...)
	return nil
}

func (g *metadataGroup) Reset() {
	g.nowEventsLength = 0
	g.nowEventsByteLength = 0
	g.Events = make([]models.PipelineEvent, 0, g.MaxEventsLength)
}

func (g *metadataGroup) evaluateByteLength(events []models.PipelineEvent) int {
	if len(events) == 0 {
		return 0
	}
	length := 0
	for _, event := range events {
		byteArray, ok := event.(models.ByteArray)
		if !ok {
			continue
		}
		length += len(byteArray)
	}
	return length
}

func NewMetadataGroup(meta models.Metadata, maxEventsLength, maxBytesLength int) *metadataGroup {
	return &metadataGroup{
		Group:               &models.GroupInfo{Metadata: meta},
		MaxEventsLength:     maxEventsLength,
		MaxEventsByteLength: maxBytesLength,
		Lock:                &sync.Mutex{},
	}
}

type AggregatorMetadataGroup struct {
	GroupMetadataKeys   []string
	GroupMaxEventLength int
	GroupMaxByteLength  int

	groupAgg sync.Map
}

func (g *AggregatorMetadataGroup) Init(context ilogtail.Context, que ilogtail.LogGroupQueue) (int, error) {
	if len(g.GroupMetadataKeys) == 0 {
		return 0, fmt.Errorf("must specify GroupMetadataKeys")
	}
	if g.GroupMaxEventLength <= 0 {
		return 0, fmt.Errorf("unknown GroupMaxEventLength")
	}
	if g.GroupMaxByteLength <= 0 {
		return 0, fmt.Errorf("unknown GroupMaxByteLength")
	}
	return 0, nil
}

func (g *AggregatorMetadataGroup) Description() string {
	return "aggregator that recombine the groupEvents by group metadata"
}

func (g *AggregatorMetadataGroup) Reset() {
	g.groupAgg.Range(func(key, value any) bool {
		metaGroup := value.(*metadataGroup)
		metaGroup.Reset()
		return true
	})
	return
}

func (g *AggregatorMetadataGroup) Record(event *models.PipelineGroupEvents, ctx ilogtail.PipelineContext) error {
	return g.getOrCreateMetadataGroup(event).Record(event, ctx)
}

func (g *AggregatorMetadataGroup) GetResult(ctx ilogtail.PipelineContext) error {
	g.groupAgg.Range(func(key, value any) bool {
		metaGroup := value.(*metadataGroup)
		if err := metaGroup.GetResult(ctx); err != nil {
			return false
		}
		return true
	})
	return nil
}

func (g *AggregatorMetadataGroup) getOrCreateMetadataGroup(event *models.PipelineGroupEvents) *metadataGroup {
	aggKey, metadata := g.buildGroupKey(event)
	group, ok := g.groupAgg.Load(aggKey)
	if !ok {
		group, _ = g.groupAgg.LoadOrStore(aggKey, NewMetadataGroup(metadata, g.GroupMaxEventLength, g.GroupMaxByteLength))
	}
	return group.(*metadataGroup)
}

func (g *AggregatorMetadataGroup) buildGroupKey(event *models.PipelineGroupEvents) (string, models.Metadata) {
	metadataValues := strings.Builder{}
	metadataKeyValues := map[string]string{}
	for index, key := range g.GroupMetadataKeys {
		value := event.Group.GetMetadata().Get(key)
		metadataKeyValues[key] = value
		if index != 0 {
			metadataValues.WriteString(connector)
		}
		metadataValues.WriteString(value)
	}
	return metadataValues.String(), models.NewMetadataWithMap(metadataKeyValues)
}

func NewAggregatorMetadataGroup() *AggregatorMetadataGroup {
	return &AggregatorMetadataGroup{
		GroupMetadataKeys:   make([]string, 0),
		GroupMaxByteLength:  MaxBytesLength,
		GroupMaxEventLength: MaxEventsLength,
	}
}

func init() {
	ilogtail.Aggregators["aggregator_metadata_group"] = func() ilogtail.Aggregator {
		return NewAggregatorMetadataGroup()
	}
}
