// Copyright 2021 iLogtail Authors
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

package entity

import (
	"sync"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
	"github.com/alibaba/ilogtail/plugins/aggregator/baseagg"
)

const (
	MaxLogCount     = 1024
	MaxLogGroupSize = 3 * 1024 * 1024
)

const DefaultEnitityLogstore = "k8s-meta-entity"
const DefaultEnitityLinkLogstore = "k8s-meta-entity-link"

type AggregatorEntity struct {
	MaxLogGroupCount    int
	MaxLogCount         int
	PackFlag            bool
	Topic               string
	EnitityLogstore     string
	EnitityLinkLogstore string

	entityAgg     *baseagg.AggregatorBase
	entityLinkAgg *baseagg.AggregatorBase

	Lock    *sync.Mutex
	context pipeline.Context
	queue   pipeline.LogGroupQueue
}

func (p *AggregatorEntity) Init(context pipeline.Context, que pipeline.LogGroupQueue) (int, error) {
	if p.EnitityLogstore == "" {
		p.EnitityLogstore = DefaultEnitityLogstore
	}
	if p.EnitityLinkLogstore == "" {
		p.EnitityLinkLogstore = DefaultEnitityLinkLogstore
	}

	p.context = context
	p.queue = que

	p.entityAgg = baseagg.NewAggregatorBase()
	if _, err := p.entityAgg.Init(context, que); err != nil {
		return 0, err
	}
	p.entityAgg.InitInner(p.PackFlag, p.context.GetConfigName()+p.EnitityLogstore+util.GetIPAddress()+time.Now().String(), p.Lock, p.EnitityLogstore, p.Topic, p.MaxLogCount, p.MaxLogGroupCount)

	p.entityLinkAgg = baseagg.NewAggregatorBase()
	if _, err := p.entityLinkAgg.Init(context, que); err != nil {
		return 0, err
	}
	p.entityLinkAgg.InitInner(p.PackFlag, p.context.GetConfigName()+p.EnitityLinkLogstore+util.GetIPAddress()+time.Now().String(), p.Lock, p.EnitityLinkLogstore, p.Topic, p.MaxLogCount, p.MaxLogGroupCount)

	return 0, nil
}

func (*AggregatorEntity) Description() string {
	return "aggregator router for entity"
}

// Add adds @log to aggregator.
// Add use first content as route key
// Add returns any error encountered, nil means success.
func (p *AggregatorEntity) Add(log *protocol.Log, ctx map[string]interface{}) error {
	if len(log.Contents) > 0 {
		routeKey := log.Contents[0]
		switch routeKey.Key {
		case "__domain__":
			return p.entityAgg.Add(log, ctx)
		case "__src_domain__":
			return p.entityLinkAgg.Add(log, ctx)
		default:
			logger.Warning(p.context.GetRuntimeContext(), "SKYWALKING_TOPIC_NOT_RECOGNIZED", "error", "topic not recognized", "topic", routeKey.Value)
			return p.entityAgg.Add(log, ctx)
		}
	}
	return nil
}

func (p *AggregatorEntity) Flush() []*protocol.LogGroup {
	logGroups := []*protocol.LogGroup{}
	logGroups = append(logGroups, p.entityAgg.Flush()...)
	logGroups = append(logGroups, p.entityLinkAgg.Flush()...)
	return logGroups
}

func (p *AggregatorEntity) Reset() {
	p.entityAgg.Reset()
	p.entityLinkAgg.Reset()
}

func NewAggregatorEntity() *AggregatorEntity {
	return &AggregatorEntity{
		MaxLogGroupCount: 4,
		MaxLogCount:      MaxLogCount,
		PackFlag:         true,
		Lock:             &sync.Mutex{},
	}
}
func init() {
	pipeline.Aggregators["aggregator_entity"] = func() pipeline.Aggregator {
		return NewAggregatorEntity()
	}
}
