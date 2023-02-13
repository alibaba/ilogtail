// Copyright 2023 iLogtail Authors
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

package opentelemetry

import (
	"context"
	"fmt"
	"sync"
	"time"

	"go.opentelemetry.io/collector/pdata/plog"
	"go.opentelemetry.io/collector/pdata/plog/plogotlp"
	"go.opentelemetry.io/collector/pdata/pmetric"
	"go.opentelemetry.io/collector/pdata/pmetric/pmetricotlp"
	"go.opentelemetry.io/collector/pdata/ptrace"
	"go.opentelemetry.io/collector/pdata/ptrace/ptraceotlp"
	"google.golang.org/grpc"
	"google.golang.org/grpc/connectivity"
	"google.golang.org/grpc/metadata"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	converter "github.com/alibaba/ilogtail/pkg/protocol/converter"

	"github.com/alibaba/ilogtail/helper"
)

type Version string

var v1 Version = "v1"

type FlusherOTLP struct {
	GrpcConfig *helper.GrpcClientConfig `json:"Grpc"`
	Version    Version                  `json:"Version"`
	Logs       *helper.GrpcClientConfig
	Metrics    *helper.GrpcClientConfig
	Traces     *helper.GrpcClientConfig

	converter *converter.Converter
	metadata  metadata.MD
	context   pipeline.Context

	grpcConn *grpc.ClientConn

	logClient    plogotlp.GRPCClient
	metricClient pmetricotlp.GRPCClient
	traceClient  ptraceotlp.GRPCClient
}

func (f *FlusherOTLP) Description() string {
	return "Open Telemetry flusher for ilogtail"
}

func (f *FlusherOTLP) Init(ctx pipeline.Context) error {
	f.context = ctx
	logger.Info(f.context.GetRuntimeContext(), "otlp flusher init", "initializing")
	convert, err := f.getConverter()
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init otlp converter fail, error", err)
		return err
	}
	f.converter = convert

	if f.GrpcConfig == nil {
		err = fmt.Errorf("GrpcConfig cannit be nil")
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init otlp converter fail, error", err)
		return err
	}

	dialOpts, err := f.GrpcConfig.GetDialOptions()
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init gRPC client options fail, error", err)
		return err
	}
	logger.Info(f.context.GetRuntimeContext(), "otlp flusher init", "initializing gRPC conn", "endpoint", f.GrpcConfig.GetEndpoint())

	if f.grpcConn, err = grpc.Dial(f.GrpcConfig.GetEndpoint(), dialOpts...); err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init otlp gRPC conn fail, error", err)
		return err
	}

	logger.Info(f.context.GetRuntimeContext(), "init otlp gRPC conn. status", f.grpcConn.GetState())
	f.metadata = metadata.New(f.GrpcConfig.Headers)

	f.logClient = plogotlp.NewGRPCClient(f.grpcConn)
	f.metricClient = pmetricotlp.NewGRPCClient(f.grpcConn)
	f.traceClient = ptraceotlp.NewGRPCClient(f.grpcConn)
	logger.Info(f.context.GetRuntimeContext(), "otlp flusher init", "initialized")
	return nil
}

func (f *FlusherOTLP) getConverter() (*converter.Converter, error) {
	switch f.Version {
	case v1:
		return converter.NewConverter(converter.ProtocolOtlpLogV1, converter.EncodingNone, nil, nil)
	default:
		return nil, fmt.Errorf("unsupported otlp log protocol version : %s", f.Version)
	}
}

func (f *FlusherOTLP) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	request := f.convertLogGroupToRequest(logGroupList)
	if !f.GrpcConfig.Retry.Enable {
		return f.flushLogs(request)
	}
	return f.flushLogsWithRetry(request)
}

// Export data to destination, such as gRPC, console, file, etc.
// It is expected to return no error at most time because IsReady will be called
// before it to make sure there is space for next data.
func (f *FlusherOTLP) Export(pipelinegroupeEventSlice []*models.PipelineGroupEvents, ctx pipeline.PipelineContext) error {
	logReq, metricReq, traceReq := f.convertPipelinesGroupeEventsToRequest(pipelinegroupeEventSlice)
	if !f.GrpcConfig.Retry.Enable {
		return f.flushRequests(logReq, metricReq, traceReq)

	}
	return f.flushRequestsWithRetry(logReq, metricReq, traceReq)
}

func (f *FlusherOTLP) convertPipelinesGroupeEventsToRequest(pipelinegroupeEventSlice []*models.PipelineGroupEvents) (plogotlp.ExportRequest, pmetricotlp.ExportRequest, ptraceotlp.ExportRequest) {
	logs := plog.NewLogs()
	metrics := pmetric.NewMetrics()
	traces := ptrace.NewTraces()

	for _, ps := range pipelinegroupeEventSlice {
		resourceLog, resourceMetric, resourceTrace, err := f.converter.ConvertPipelineGroupEventsToOTLPEvents(ps)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init gRPC client options fail, error", err)
		}
		if resourceLog.ScopeLogs().Len() > 0 {
			newLog := logs.ResourceLogs().AppendEmpty()
			resourceLog.MoveTo(newLog)
		}

		if resourceMetric.ScopeMetrics().Len() > 0 {
			newMetric := metrics.ResourceMetrics().AppendEmpty()
			resourceMetric.MoveTo(newMetric)
		}

		if resourceTrace.ScopeSpans().Len() > 0 {
			newTrace := traces.ResourceSpans().AppendEmpty()
			resourceTrace.MoveTo(newTrace)
		}
	}

	return plogotlp.NewExportRequestFromLogs(logs),
		pmetricotlp.NewExportRequestFromMetrics(metrics),
		ptraceotlp.NewExportRequestFromTraces(traces)
}

func (f *FlusherOTLP) convertLogGroupToRequest(logGroupList []*protocol.LogGroup) plogotlp.ExportRequest {
	logs := plog.NewLogs()
	for _, logGroup := range logGroupList {
		c, _ := f.converter.Do(logGroup)
		if log, ok := c.(plog.ResourceLogs); ok {
			if log.ScopeLogs().Len() > 0 {
				newLog := logs.ResourceLogs().AppendEmpty()
				log.MoveTo(newLog)
			}
		}
	}

	return plogotlp.NewExportRequestFromLogs(logs)
}

func (f *FlusherOTLP) flushRequests(log plogotlp.ExportRequest, metric pmetricotlp.ExportRequest, trace ptraceotlp.ExportRequest) error {
	wg := sync.WaitGroup{}
	errChan := make(chan error, 3)

	wg.Add(1)
	go func() {
		defer wg.Done()
		errChan <- f.flushLogs(log)
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		errChan <- f.flushMetrics(metric)
	}()

	wg.Add(1)
	go func() {
		defer wg.Done()
		errChan <- f.flushTraces(trace)
	}()

	wg.Wait()

	return <-errChan
}

func (f *FlusherOTLP) flushLogs(request plogotlp.ExportRequest) error {
	if err := f.timeoutFlushLogs(request); err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send data to otlp server fail, error", err)
		return err
	}
	return nil
}

func (f *FlusherOTLP) flushMetrics(request pmetricotlp.ExportRequest) error {
	if err := f.timeoutFlushMetrics(request); err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send data to otlp server fail, error", err)
		return err
	}
	return nil
}

func (f *FlusherOTLP) flushTraces(request ptraceotlp.ExportRequest) error {
	if err := f.timeoutFlushTraces(request); err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send data to otlp server fail, error", err)
		return err
	}
	return nil
}

func (f *FlusherOTLP) flushRequestsWithRetry(log plogotlp.ExportRequest, metric pmetricotlp.ExportRequest, trace ptraceotlp.ExportRequest) error {
	wg := sync.WaitGroup{}
	errChan := make(chan error, 3)

	wg.Add(3)
	go func() {
		defer wg.Done()
		errChan <- f.flushMetricsWithRetry(metric)
	}()

	go func() {
		defer wg.Done()
		errChan <- f.flushLogsWithRetry(log)
	}()

	go func() {
		defer wg.Done()
		errChan <- f.flushTracesWithRetry(trace)
	}()

	wg.Wait()
	return <-errChan
}

func (f *FlusherOTLP) flushLogsWithRetry(request plogotlp.ExportRequest) error {
	var retryNum = 0

	for {
		err := f.timeoutFlushLogs(request)
		if err == nil {
			return nil
		}
		retry := helper.GetRetryInfo(err)
		if retry == nil || retryNum >= f.GrpcConfig.Retry.MaxCount {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send data to otlp server fail, error", err)
			return err
		}
		logger.Warning(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send data to otlp server fail, will retry...")
		retryNum++
		<-time.After(retry.ShouldDelay(f.GrpcConfig.Retry.DefaultDelay))
	}
}

func (f *FlusherOTLP) flushMetricsWithRetry(request pmetricotlp.ExportRequest) error {
	var retryNum = 0

	for {
		err := f.timeoutFlushMetrics(request)
		if err == nil {
			return nil
		}
		retry := helper.GetRetryInfo(err)
		if retry == nil || retryNum >= f.GrpcConfig.Retry.MaxCount {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send data to otlp server fail, error", err)
			return err
		}
		logger.Warning(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send data to otlp server fail, will retry...")
		retryNum++
		<-time.After(retry.ShouldDelay(f.GrpcConfig.Retry.DefaultDelay))
	}
}

func (f *FlusherOTLP) flushTracesWithRetry(request ptraceotlp.ExportRequest) error {
	var retryNum = 0

	for {
		err := f.timeoutFlushTraces(request)
		if err == nil {
			return nil
		}
		retry := helper.GetRetryInfo(err)
		if retry == nil || retryNum >= f.GrpcConfig.Retry.MaxCount {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send data to otlp server fail, error", err)
			return err
		}
		logger.Warning(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send data to otlp server fail, will retry...")
		retryNum++
		<-time.After(retry.ShouldDelay(f.GrpcConfig.Retry.DefaultDelay))
	}
}

func (f *FlusherOTLP) timeoutFlushLogs(request plogotlp.ExportRequest) error {
	timeout, canal := context.WithTimeout(f.withMetadata(), f.GrpcConfig.GetTimeout())
	defer canal()
	_, err := f.logClient.Export(timeout, request)
	return err
}

func (f *FlusherOTLP) timeoutFlushMetrics(request pmetricotlp.ExportRequest) error {
	timeout, canal := context.WithTimeout(f.withMetadata(), f.GrpcConfig.GetTimeout())
	defer canal()
	_, err := f.metricClient.Export(timeout, request)
	return err
}

func (f *FlusherOTLP) timeoutFlushTraces(request ptraceotlp.ExportRequest) error {
	timeout, canal := context.WithTimeout(f.withMetadata(), f.GrpcConfig.GetTimeout())
	defer canal()
	_, err := f.traceClient.Export(timeout, request)
	return err
}

// append withMetadata to context. refer to https://github.com/open-telemetry/opentelemetry-collector/blob/main/exporter/otlpexporter/otlp.go#L121
func (f *FlusherOTLP) withMetadata() context.Context {
	if f.metadata.Len() > 0 {
		return metadata.NewOutgoingContext(context.Background(), f.metadata)
	}
	return context.Background()
}

func (f *FlusherOTLP) SetUrgent(flag bool) {
}

// IsReady is ready to flush
func (f *FlusherOTLP) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	if f.grpcConn == nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_READY_ALARM", "otlp flusher is not ready, gRPC conn is nil")
		return false
	}
	state := f.grpcConn.GetState()
	var ready bool
	switch f.grpcConn.GetState() {
	case connectivity.Ready:
		ready = true
	case connectivity.Connecting:
		if !f.GrpcConfig.WaitForReady {
			ready = false
			break
		}
		timeout, canal := context.WithTimeout(context.Background(), f.GrpcConfig.GetTimeout())
		defer canal()
		_ = f.grpcConn.WaitForStateChange(timeout, state)
		state = f.grpcConn.GetState()
		ready = state == connectivity.Ready
	default:
		logger.Warning(f.context.GetRuntimeContext(), "FLUSHER_READY_ALARM", "gRPC conn is not ready, will reconnect. current status", f.grpcConn.GetState())
		state = f.reConnect()
		ready = state == connectivity.Ready
	}
	if !ready {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_READY_ALARM", "otlp flusher is not ready. current status", f.grpcConn.GetState())
	}
	return ready
}

func (f *FlusherOTLP) reConnect() connectivity.State {
	f.grpcConn.Connect()
	return f.grpcConn.GetState()
}

// Stop ...
func (f *FlusherOTLP) Stop() error {
	err := f.grpcConn.Close()
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_STOP_ALARM", "stop otlp flusher fail, error", err)
	}
	return err
}

func (f *FlusherOTLP) checkConfig() error {
	if f.GrpcConfig == nil {
		if f.Logs == nil && f.Metrics == nil && f.Traces == nil {
			return fmt.Errorf("empty_grpc_config")
		}
	}
	return nil
}

func init() {
	pipeline.Flushers["flusher_otlp"] = func() pipeline.Flusher {
		return &FlusherOTLP{Version: v1}
	}
}
