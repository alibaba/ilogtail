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

package context

import (
	"strings"
	"sync"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
)

const (
	MaxLogCount         = 1024
	MaxLogGroupSize     = 3 * 1024 * 1024
	SmallPackIDTimeout  = time.Duration(24*30) * time.Hour
	BigPackIDTimeout    = time.Duration(24) * time.Hour
	PackIDMapLenThresh  = 100000
	MaxLogGroupPoolSize = 10 * 1024 * 1024
)

type LogPackSeqInfo struct {
	seq            int64
	lastUpdateTime time.Time
}

type AggregatorContext struct {
	MaxLogGroupCount                 int    // the maximum log group count per source to trigger flush operation
	MaxLogCount                      int    // the maximum log in a log group
	Topic                            string // the output topic
	ContextPreservationToleranceSize int    // the maximum number of log source per config where logGroupPoolMap will not be cleared periodically
	PackFlag                         bool   // whether to add __pack_id__ as a tag

	lock               *sync.Mutex
	logGroupPoolMap    map[string][]*protocol.LogGroup
	nowLogGroupSizeMap map[string]int
	logGroupPoolSize   int
	packIDMap          map[string]*LogPackSeqInfo
	defaultPack        string
	context            pipeline.Context
	queue              pipeline.LogGroupQueue

	packIDMapCleanInterval time.Duration
	packIDTimeout          time.Duration
	lastCleanPackIDMapTime time.Time
}

// Init method would be trigger before working.
// 1. context store the metadata of this Logstore config
// 2. que is a transfer channel for flushing LogGroup when reaches the maximum in the cache.
func (p *AggregatorContext) Init(context pipeline.Context, que pipeline.LogGroupQueue) (int, error) {
	p.defaultPack = util.NewPackIDPrefix(context.GetConfigName())
	p.context = context
	p.queue = que
	p.lastCleanPackIDMapTime = time.Now()
	return 0, nil
}

func (*AggregatorContext) Description() string {
	return "context aggregator for logtail"
}

// Add adds @log with @ctx to aggregator.
func (p *AggregatorContext) Add(log *protocol.Log, ctx map[string]interface{}) error {
	p.lock.Lock()
	defer p.lock.Unlock()

	source := p.defaultPack
	if _, ok := ctx["source"]; ok {
		source = ctx["source"].(string)
		if !strings.HasSuffix(source, "-") {
			source += "-"
		}
	}

	topic := p.Topic
	if _, ok := ctx["topic"]; ok {
		topic = ctx["topic"].(string)
	}

	logGroupList := p.logGroupPoolMap[source]
	if logGroupList == nil {
		logGroupList = make([]*protocol.LogGroup, 0, p.MaxLogGroupCount)
	}
	if len(logGroupList) == 0 {
		if _, ok := ctx["tags"]; ok {
			newLogGroup := p.newLogGroup(source, topic)
			fillTags(ctx["tags"].([]*protocol.LogTag), newLogGroup)
			logGroupList = append(logGroupList, newLogGroup)
		} else {
			logGroupList = append(logGroupList, p.newLogGroup(source, topic))
		}
	}
	nowLogGroup := logGroupList[len(logGroupList)-1]

	logSize := p.evaluateLogSize(log)
	// When current log group is full (log count or no more capacity for current log),
	// allocate a new log group.
	if len(nowLogGroup.Logs) >= p.MaxLogCount || p.nowLogGroupSizeMap[source]+logSize > MaxLogGroupSize {
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
		p.nowLogGroupSizeMap[source] = 0
		if _, ok := ctx["tags"]; ok {
			newLogGroup := p.newLogGroup(source, topic)
			fillTags(ctx["tags"].([]*protocol.LogTag), newLogGroup)
			logGroupList = append(logGroupList, newLogGroup)
		} else {
			logGroupList = append(logGroupList, p.newLogGroup(source, topic))
		}
		nowLogGroup = logGroupList[len(logGroupList)-1]
	}

	// add log size
	p.nowLogGroupSizeMap[source] += logSize
	p.logGroupPoolSize += logSize
	nowLogGroup.Logs = append(nowLogGroup.Logs, log)
	p.logGroupPoolMap[source] = logGroupList
	if p.logGroupPoolSize > MaxLogGroupPoolSize {
		logGroups := p.Flush()
		var err error
		for tryCount := 1; true; tryCount++ {
			errorLogGroups := make([]*protocol.LogGroup, 0, len(logGroups))
			for i, logGroup := range logGroups {
				if len(logGroup.Logs) == 0 {
					continue
				}
				// Quick flush to avoid becoming bottleneck when large logs come.
				if err = p.queue.Add(logGroup); err == nil {
				} else {
					errorLogGroups = append(errorLogGroups, logGroups[i])
				}
			}
			if len(errorLogGroups) == 0 {
				break
			}
			// wait until shutdown is active
			if tryCount%100 == 0 {
				logger.Warning(p.context.GetRuntimeContext(), "AGGREGATOR_ADD_ALARM", "error", err)
			}
			time.Sleep(time.Millisecond * 10)
			logGroups = errorLogGroups
		}

	}
	return nil
}

func fillTags(logTags []*protocol.LogTag, logGroup *protocol.LogGroup) {
	logGroup.LogTags = append(logGroup.LogTags, logTags...)
}

// Flush ...
func (p *AggregatorContext) Flush() []*protocol.LogGroup {
	p.lock.Lock()
	defer p.lock.Unlock()
	var ret []*protocol.LogGroup
	for pack, logGroupList := range p.logGroupPoolMap {
		if len(logGroupList) == 0 {
			continue
		}
		ret = append(ret, logGroupList...)
		delete(p.logGroupPoolMap, pack)
		delete(p.nowLogGroupSizeMap, pack)
	}
	p.logGroupPoolSize = 0

	curTime := time.Now()
	if len(p.packIDMap) > p.ContextPreservationToleranceSize && time.Since(p.lastCleanPackIDMapTime) > p.packIDMapCleanInterval {
		if len(p.packIDMap) > PackIDMapLenThresh {
			p.packIDTimeout = BigPackIDTimeout
		} else {
			p.packIDTimeout = SmallPackIDTimeout
		}
		for k, v := range p.packIDMap {
			if curTime.Sub(v.lastUpdateTime) > p.packIDTimeout {
				delete(p.packIDMap, k)
			}
		}
		p.lastCleanPackIDMapTime = curTime
	}

	return ret
}

// Reset ...
func (p *AggregatorContext) Reset() {
	p.lock.Lock()
	defer p.lock.Unlock()
	p.logGroupPoolMap = make(map[string][]*protocol.LogGroup)
}

func (p *AggregatorContext) newLogGroup(pack string, topic string) *protocol.LogGroup {
	logGroup := &protocol.LogGroup{
		Logs:  make([]*protocol.Log, 0, p.MaxLogCount),
		Topic: topic,
	}
	info := p.packIDMap[pack]
	if info == nil {
		info = &LogPackSeqInfo{
			seq: 1,
		}
	}
	if p.PackFlag {
		logGroup.LogTags = append(logGroup.LogTags, util.NewLogTagForPackID(pack, &info.seq))
	}
	info.lastUpdateTime = time.Now()
	p.packIDMap[pack] = info

	return logGroup
}

func (*AggregatorContext) evaluateLogSize(log *protocol.Log) int {
	var logSize = 6
	for _, logC := range log.Contents {
		logSize += 5 + len(logC.Key) + len(logC.Value)
	}
	return logSize
}

// NewAggregatorContext create a default aggregator with default value.
func NewAggregatorContext() *AggregatorContext {
	return &AggregatorContext{
		MaxLogGroupCount:                 2,
		MaxLogCount:                      MaxLogCount,
		ContextPreservationToleranceSize: 10,
		PackFlag:                         true,
		logGroupPoolMap:                  make(map[string][]*protocol.LogGroup),
		nowLogGroupSizeMap:               make(map[string]int),
		packIDMap:                        make(map[string]*LogPackSeqInfo),
		packIDMapCleanInterval:           time.Duration(600) * time.Second,
		lock:                             &sync.Mutex{},
	}
}

// Register the plugin to the Aggregators array.
func init() {
	pipeline.Aggregators["aggregator_context"] = func() pipeline.Aggregator {
		return NewAggregatorContext()
	}
}
