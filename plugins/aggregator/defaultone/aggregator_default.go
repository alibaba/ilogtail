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

package defaultone

import (
	"strings"
	"sync"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
)

const (
	MaxLogCount     = 1024
	MaxLogGroupSize = 3 * 1024 * 1024
)

// AggregatorDefault is the default aggregator in plugin system.
// If there is no specific aggregator in plugin config, it will be added.
type AggregatorDefault struct {
	MaxLogGroupCount int    // the maximum log group count to trigger flush operation
	MaxLogCount      int    // the maximum log in a log group
	Topic            string // the output topic

	Lock            *sync.Mutex
	logGroupPool    map[string][]*protocol.LogGroup
	nowLogGroupSize map[string]int
	packID          map[string]int64
	defaultPack     string
	context         ilogtail.Context
	queue           ilogtail.LogGroupQueue
}

// Init method would be trigger before working.
// 1. context store the metadata of this Logstore config
// 2. que is a transfer channel for flushing LogGroup when reaches the maximum in the cache.
func (p *AggregatorDefault) Init(context ilogtail.Context, que ilogtail.LogGroupQueue) (int, error) {
	p.defaultPack = util.NewPackIDPrefix(context.GetConfigName())
	p.context = context
	p.queue = que
	return 0, nil
}

func (*AggregatorDefault) Description() string {
	return "default aggregator for logtail"
}

// Add adds @log with @ctx to aggregator.
func (p *AggregatorDefault) Add(log *protocol.Log, ctx map[string]interface{}) error {
	p.Lock.Lock()
	defer p.Lock.Unlock()

	source := p.defaultPack
	if _, ok := ctx["source"]; ok {
		source = ctx["source"].(string)
		if !strings.HasSuffix(source, "-") {
			source += "-"
		}
	}

	logGroupList := p.logGroupPool[source]
	if len(logGroupList) == 0 {
		logGroupList = append(logGroupList, p.newLogGroup(source))
	}
	nowLogGroup := logGroupList[len(logGroupList)-1]

	logSize := p.evaluateLogSize(log)
	// When current log group is full (log count or no more capacity for current log),
	// allocate a new log group.
	if len(nowLogGroup.Logs) >= p.MaxLogCount || p.nowLogGroupSize[source]+logSize > MaxLogGroupSize {
		// The number of log group exceeds limit, make a quick flush.
		if len(logGroupList) == p.MaxLogGroupCount {
			// Quick flush to avoid becoming bottleneck when large logs come.
			if err := p.queue.Add(logGroupList[0]); err == nil {
				// add success, remove head log group
				logGroupList = logGroupList[1:]
			} else {
				return err
			}
		}
		// New log group, reset size.
		logGroupList = append(logGroupList, p.newLogGroup(source))
		nowLogGroup = logGroupList[len(logGroupList)-1]
	}

	// add log size
	p.nowLogGroupSize[source] += logSize
	nowLogGroup.Logs = append(nowLogGroup.Logs, log)
	p.logGroupPool[source] = logGroupList
	return nil
}

// Flush ...
func (p *AggregatorDefault) Flush() (ret []*protocol.LogGroup) {
	p.Lock.Lock()
	defer p.Lock.Unlock()
	for pack, logGroupList := range p.logGroupPool {
		if len(logGroupList) == 0 {
			continue
		}
		ret = append(ret, logGroupList[0:len(logGroupList)-1]...)
		// check if last is empty
		if len(logGroupList[len(logGroupList)-1].Logs) != 0 {
			ret = append(ret, logGroupList[len(logGroupList)-1])
		}
		p.logGroupPool[pack] = make([]*protocol.LogGroup, 0, p.MaxLogGroupCount)
		p.nowLogGroupSize[pack] = 0
	}
	return
}

// Reset ...
func (p *AggregatorDefault) Reset() {
	p.Lock.Lock()
	defer p.Lock.Unlock()
	p.logGroupPool = make(map[string][]*protocol.LogGroup)
}

func (p *AggregatorDefault) newLogGroup(pack string) *protocol.LogGroup {
	logGroup := &protocol.LogGroup{
		Logs:  make([]*protocol.Log, 0, p.MaxLogCount),
		Topic: p.Topic,
	}
	// addPackID adds __pack_id__ into logGroup.LogTags if it is not existing at last.
	if len(logGroup.LogTags) == 0 || logGroup.LogTags[len(logGroup.LogTags)-1].GetKey() != util.PackIDTagKey {
		id := p.packID[pack]
		logGroup.LogTags = append(logGroup.LogTags, util.NewLogTagForPackID(pack, &id))
		p.packID[pack] = id
	}
	p.nowLogGroupSize[pack] = 0

	return logGroup
}

func (*AggregatorDefault) evaluateLogSize(log *protocol.Log) int {
	var logSize = 6
	for _, logC := range log.Contents {
		logSize += 5 + len(logC.Key) + len(logC.Value)
	}
	return logSize
}

// NewAggregatorDefault create a default aggregator with default value.
func NewAggregatorDefault() *AggregatorDefault {
	return &AggregatorDefault{
		MaxLogGroupCount: 4,
		MaxLogCount:      MaxLogCount,
		logGroupPool:     make(map[string][]*protocol.LogGroup),
		nowLogGroupSize:  make(map[string]int),
		packID:           make(map[string]int64),
		Lock:             &sync.Mutex{},
	}
}

// Register the plugin to the Aggregators array.
func init() {
	ilogtail.Aggregators["aggregator_default"] = func() ilogtail.Aggregator {
		return NewAggregatorDefault()
	}
}
