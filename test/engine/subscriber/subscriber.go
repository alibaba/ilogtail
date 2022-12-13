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

package subscriber

import (
	"context"
	"errors"
	"fmt"
	"net/url"
	"sync"

	"github.com/alibaba/ilogtail/pkg/doc"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/test/engine/boot"
)

var factory = make(map[string]Creator)
var mu sync.Mutex

// Creator creates a new subscriber instance according to the spec.
type Creator func(spec map[string]interface{}) (Subscriber, error)

// Subscriber receives the logs transfer by ilogtail.
type Subscriber interface {
	doc.Doc
	// Name of subscriber
	Name() string
	// Start the subscriber
	Start() error
	// Stop the subscriber
	Stop()
	// SubscribeChan is subscribed by the external component to get telemetry dat.
	SubscribeChan() <-chan *protocol.LogGroup
	// FlusherConfig returns the default flusher config for Ilogtail container to transfer the received or self telemetry data.
	FlusherConfig() string
}

func RegisterCreator(name string, creator Creator) {
	mu.Lock()
	defer mu.Unlock()
	factory[name] = creator
}

func New(name string, cfg map[string]interface{}) (Subscriber, error) {
	mu.Lock()
	defer mu.Unlock()
	creator, ok := factory[name]
	if !ok {
		return nil, fmt.Errorf("cannot find %s subscriber", name)
	}
	return creator(cfg)
}

func TryReplacePhysicalAddress(addr string) (string, error) {
	uri, err := url.Parse(addr)
	if err != nil {
		return addr, errors.New("invalid uri: " + addr)
	}
	if uri.Port() == "" {
		uri.Host += ":80"
	}

	physicalAddr := boot.GetPhysicalAddress(uri.Host)
	logger.Debugf(context.Background(), "subscriber get physical address result: %s -> %s", uri.Host, physicalAddr)
	if len(physicalAddr) == 0 {
		return addr, nil
	}
	uri.Host = physicalAddr
	return uri.String(), nil
}
