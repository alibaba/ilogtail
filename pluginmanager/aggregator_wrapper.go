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

package pluginmanager

import (
	"errors"
	"sync"
	"time"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
)

var errAggAdd = errors.New("loggroup queue is full")

// AggregatorWrapper wrappers Aggregator.
// It implements LogGroupQueue interface, and is passed to associated Aggregator.
// Aggregator uses Add function to pass log groups to wrapper, and then wrapper
// passes log groups to associated LogstoreConfig through channel LogGroupsChan.
// In fact, LogGroupsChan == (associated) LogstoreConfig.LogGroupsChan.
type AggregatorWrapper struct {
	Aggregator    ilogtail.Aggregator
	Config        *LogstoreConfig
	LogGroupsChan chan *protocol.LogGroup
	Interval      time.Duration

	PipeContext ilogtail.PipelineContext

	shutdown  chan struct{}
	waitgroup sync.WaitGroup
}

// Add inserts @loggroup to LogGroupsChan if @loggroup is not empty.
// It is called by associated Aggregator.
// It returns errAggAdd when queue is full.
func (p *AggregatorWrapper) Add(loggroup *protocol.LogGroup) error {
	if len(loggroup.Logs) == 0 {
		return nil
	}
	select {
	case p.LogGroupsChan <- loggroup:
		return nil
	default:
		return errAggAdd
	}
}

// AddWithWait inserts @loggroup to LogGroupsChan, and it waits at most @duration.
// It works like Add but adds a timeout policy when log group queue is full.
// It returns errAggAdd when queue is full and timeout.
// NOTE: no body calls it now.
func (p *AggregatorWrapper) AddWithWait(loggroup *protocol.LogGroup, duration time.Duration) error {
	if len(loggroup.Logs) == 0 {
		return nil
	}
	timer := time.NewTimer(duration)
	select {
	case p.LogGroupsChan <- loggroup:
		return nil
	case <-timer.C:
		return errAggAdd
	}
}

// Run calls periodically Aggregator.Flush to get log groups from associated aggregator and
// pass them to LogstoreConfig through LogGroupsChan.
func (p *AggregatorWrapper) Run() {
	defer panicRecover(p.Aggregator.Description())
	p.shutdown = make(chan struct{}, 1)
	p.waitgroup.Add(1)
	defer p.waitgroup.Done()
	for {
		exitFlag := util.RandomSleep(p.Interval, 0.1, p.shutdown)
		if slsAggregator, ok := p.Aggregator.(ilogtail.SlsAggregator); ok {
			logGroups := slsAggregator.FlushLogs()
			for _, logGroup := range logGroups {
				if len(logGroup.Logs) == 0 {
					continue
				}
				p.LogGroupsChan <- logGroup
			}
		} else if pipeAggregator, ok := p.Aggregator.(ilogtail.PipelineAggregator); ok {
			_ = pipeAggregator.Flush(p.PipeContext)
		} else {
			return
		}

		if exitFlag {
			return
		}
	}
}

func (p *AggregatorWrapper) Stop() {
	close(p.shutdown)
	p.waitgroup.Wait()
}
