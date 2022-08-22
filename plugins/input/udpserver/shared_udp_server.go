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

package udpserver

import (
	"context"
	"strings"
	"sync"
	"time"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

const labelName = "__labels__"

// SharedUDPServer used for multi configs, such as using in JMX fetcher
type SharedUDPServer struct {
	DispatchKey string
	lastLog     time.Time
	udp         *UDPServer
	collectors  map[string]ilogtail.Collector
	lock        sync.RWMutex // mutex for register collector
}

func NewSharedUDPServer(context ilogtail.Context, format, addr, dispatchKey string, maxBufferSize int) (*SharedUDPServer, error) {
	s := &UDPServer{
		Format:        format,
		Address:       addr,
		MaxBufferSize: maxBufferSize,
	}
	if _, err := s.Init(context); err != nil {
		return nil, err
	}
	server := &SharedUDPServer{
		DispatchKey: dispatchKey,
		collectors:  map[string]ilogtail.Collector{},
		lastLog:     time.Now(),
		udp:         s,
	}
	return server, nil
}

func (s *SharedUDPServer) IsRunning() bool {
	return s.udp.conn != nil
}

func (s *SharedUDPServer) Start() error {
	return s.udp.doStart(s.dispatcher)
}

func (s *SharedUDPServer) Stop() error {
	return s.udp.Stop()
}

func (s *SharedUDPServer) UnregisterCollectors(key string) {
	s.lock.Lock()
	defer s.lock.Unlock()
	delete(s.collectors, key)
}

func (s *SharedUDPServer) RegisterCollectors(key string, collector ilogtail.Collector) {
	s.lock.Lock()
	defer s.lock.Unlock()
	s.collectors[key] = collector
}

func (s *SharedUDPServer) dispatcher(logs []*protocol.Log) {
	for _, log := range logs {
		if logger.DebugFlag() {
			logger.Debug(context.Background(), "log", log.String())
		}
		tag := s.cutDispatchTag(log)
		if tag == "" {
			continue
		}
		s.lock.RLock()
		collector, ok := s.collectors[tag]
		s.lock.RUnlock()
		if !ok {
			s.logErr("collector not exist for " + tag)
			continue
		}
		collector.AddRawLog(log)
	}
}

func (s *SharedUDPServer) logErr(errStr string) {

	if time.Since(s.lastLog).Seconds() > 10 {
		logger.Error(s.udp.context.GetRuntimeContext(), "MULTI_HTTP_SERVER_ALARM", "not exist dispatch key", errStr)
		s.lastLog = time.Now()
	}
}

func (s *SharedUDPServer) cutDispatchTag(log *protocol.Log) (tag string) {
	for _, content := range log.Contents {
		if content.Key == "__labels__" {
			startIdx := strings.Index(content.Value, s.DispatchKey)
			if startIdx == -1 {
				s.logErr("dispatch key not exist")
				break
			}
			endIdx := startIdx + len(s.DispatchKey) + len("#$#")
			if endIdx >= len(content.Value) {
				s.logErr("dispatch key over range")
				break
			}
			str := content.Value[endIdx:]
			offset := strings.Index(str, "|")
			if offset == -1 {
				tag = str
				content.Value = content.Value[:startIdx-1]
			} else {
				tag = str[:offset]
				content.Value = content.Value[:startIdx] + content.Value[endIdx+offset+1:]
			}
			break
		}
	}
	if tag == "" {
		s.logErr("not find any dispatch key" + log.String())
	}
	return
}
