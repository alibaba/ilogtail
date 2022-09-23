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
	"context"
	"time"

	otlpv1 "go.opentelemetry.io/proto/otlp/collector/logs/v1"
	logv1 "go.opentelemetry.io/proto/otlp/logs/v1"
	"google.golang.org/grpc"
	"google.golang.org/grpc/connectivity"
	"google.golang.org/grpc/metadata"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
	converter "github.com/alibaba/ilogtail/pkg/protocol/converter"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/helper"
)

type FlusherOTLPLog struct {
	GrpcConfig *helper.GrpcClientConfig `json:"grpc"`

	converter *converter.Converter
	metadata  metadata.MD
	context   ilogtail.Context
	grpcConn  *grpc.ClientConn
	logClient otlpv1.LogsServiceClient
}

func (f *FlusherOTLPLog) Description() string {
	return "OTLP Log flusher for logtail"
}

func (f *FlusherOTLPLog) Init(context ilogtail.Context) error {
	f.context = context
	logger.Info(f.context.GetRuntimeContext(), "otlplog flusher init", "initializing")
	convert, err := converter.NewConverter(converter.ProtocolOtlpLog, converter.EncodingNone, map[string]string{}, map[string]string{})
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init otlp_log converter fail, error", err)
		return err
	}
	f.converter = convert

	opts, err := f.GrpcConfig.GetOptions()
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init gRPC client options fail, error", err)
		return err
	}
	logger.Info(f.context.GetRuntimeContext(), "otlplog flusher init", "initializing gRPC conn", "endpoint", f.GrpcConfig.GetEndpoint())
	if f.grpcConn, err = grpc.DialContext(f.context.GetRuntimeContext(), f.GrpcConfig.GetEndpoint(), opts...); err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init otlplog gRPC conn fail, error", err)
		return err
	}
	f.metadata = metadata.New(f.GrpcConfig.Headers)
	f.logClient = otlpv1.NewLogsServiceClient(f.grpcConn)
	logger.Info(f.context.GetRuntimeContext(), "otlplog flusher init", "initialized")
	return nil
}

func (f *FlusherOTLPLog) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	resourceLogs := make([]*logv1.ResourceLogs, 0)
	for _, logGroup := range logGroupList {
		c, _ := f.converter.Do(logGroup)
		if log, ok := c.(*logv1.ResourceLogs); ok {
			resourceLogs = append(resourceLogs, log)
		}
	}
	request := &otlpv1.ExportLogsServiceRequest{
		ResourceLogs: resourceLogs,
	}
	if !f.GrpcConfig.Retry.Enable {
		f.flush(request)
	} else {
		f.flushWithRetry(request)
	}
	return nil
}

func (f *FlusherOTLPLog) flush(request *otlpv1.ExportLogsServiceRequest) {
	if _, err := f.logClient.Export(f.enhanceContext(f.context.GetRuntimeContext()), request); err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send data to otlplog server fail, error", err)
	}
}

func (f *FlusherOTLPLog) flushWithRetry(request *otlpv1.ExportLogsServiceRequest) {
	var retryNum = 0
	for {
		_, err := f.logClient.Export(f.enhanceContext(f.context.GetRuntimeContext()), request)
		if err == nil {
			return
		}
		retry := helper.GetRetryInfo(err)
		if retry == nil || retryNum >= f.GrpcConfig.Retry.MaxCount {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send data to otlplog server fail, error", err)
			return
		}
		logger.Warning(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send data to otlplog server fail, will retry...")
		retryNum++
		<-time.After(retry.ShouldDelay(f.GrpcConfig.Retry.DefaultDelay))
	}
}

// append metadata to context. refer to https://github.com/open-telemetry/opentelemetry-collector/blob/main/exporter/otlpexporter/otlp.go#L121
func (f *FlusherOTLPLog) enhanceContext(ctx context.Context) context.Context {
	if f.metadata.Len() > 0 {
		return metadata.NewOutgoingContext(ctx, f.metadata)
	}
	return ctx
}

func (f *FlusherOTLPLog) SetUrgent(flag bool) {
}

// IsReady is ready to flush
func (f *FlusherOTLPLog) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	ready := f.grpcConn != nil && f.grpcConn.GetState() == connectivity.Ready
	if !ready {
		logger.Warning(f.context.GetRuntimeContext(), "FLUSHER_READY_ALARM", "otlplog flusher is not ready. status", f.grpcConn.GetState())
	}
	return ready
}

// Stop ...
func (f *FlusherOTLPLog) Stop() error {
	err := f.grpcConn.Close()
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_STOP_ALARM", "stop otlplog flusher fail, error", err)
	}
	return err
}

func init() {
	ilogtail.Flushers["flusher_otlp_log"] = func() ilogtail.Flusher {
		return &FlusherOTLPLog{}
	}
}
