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

package otlplog

import (
	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/grpc_config"
	"github.com/alibaba/ilogtail/pkg/protocol"
	otlpClient "go.opentelemetry.io/proto/otlp/collector/logs/v1"
)

type FlusherOTLPLog struct {
	GrpcConfig *grpc_config.GrpcClientConfig `json:"grpc"`

	context   ilogtail.Context
	logClient *otlpClient.LogsServiceClient
}

func (f *FlusherOTLPLog) Description() string {
	return "OTLP Log flusher for logtail"
}

func (f *FlusherOTLPLog) Init(context ilogtail.Context) error {
	f.context = context
	return nil
}

func (f *FlusherOTLPLog) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {

	return nil
}

func (f *FlusherOTLPLog) SetUrgent(flag bool) {
}

// IsReady is ready to flush
func (f *FlusherOTLPLog) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	return true
}

// Stop ...
func (f *FlusherOTLPLog) Stop() error {
	return nil
}

func init() {
	ilogtail.Flushers["flusher_otlp_log"] = func() ilogtail.Flusher {
		return &FlusherOTLPLog{}
	}
}
