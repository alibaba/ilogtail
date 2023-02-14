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
	"reflect"
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

var (
	v1             Version = "v1"
	errEmptyConfig         = fmt.Errorf("empty_config")
	errNoEndpoint          = fmt.Errorf("no_specific_endpoint")
)

type FlusherOTLP struct {
	GrpcConfig *helper.GrpcClientConfig `json:"Grpc"`
	Version    Version                  `json:"Version"`
	Logs       *GrpcClientConfig        `json:"Logs"`
	Metrics    *GrpcClientConfig        `json:"Metrics"`
	Traces     *GrpcClientConfig        `json:"Traces"`

	converter *converter.Converter
	metadata  metadata.MD
	context   pipeline.Context

	grpcConn     *grpc.ClientConn // default conn
	logClient    *grpcClient[plogotlp.GRPCClient]
	metricClient *grpcClient[pmetricotlp.GRPCClient]
	traceClient  *grpcClient[ptraceotlp.GRPCClient]
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

	logClientConfig, logErr := mergeGrpcConfig(f.GrpcConfig, f.Logs)
	metricClientConfig, metricErr := mergeGrpcConfig(f.GrpcConfig, f.Metrics)
	traceClientConfig, traceErr := mergeGrpcConfig(f.GrpcConfig, f.Traces)

	if logErr != nil && metricErr != nil && traceErr != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init otlp converter fail, no invalid gRPC Config",
			"log config error", logErr, "metric config error", metricErr, "trace config error", traceErr)
		return logErr
	}

	if f.GrpcConfig != nil {
		f.grpcConn, err = buildGrpcClientConn(f.GrpcConfig)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init otlp gRPC conn fail, error", err)
		}
		f.metadata = metadata.New(f.GrpcConfig.Headers)
		logger.Info(f.context.GetRuntimeContext(), "otlp flusher idefault endpoint", f.GrpcConfig.Endpoint)
	}

	if logErr == nil {
		if reflect.DeepEqual(f.GrpcConfig, logClientConfig) && f.grpcConn != nil {
			logger.Info(f.context.GetRuntimeContext(), "otlp logs flusher endpoint", f.GrpcConfig.Endpoint)
			f.logClient = newGrpcClient(plogotlp.NewGRPCClient(f.grpcConn), f.grpcConn, logClientConfig, f.metadata)
		} else {
			grpcConn, err := buildGrpcClientConn(logClientConfig)
			if err != nil {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init otlp log gRPC conn fail, error", err)
			}
			logger.Info(f.context.GetRuntimeContext(), "otlp logs flusher endpoint", logClientConfig.Endpoint)
			logMeta := metadata.New(logClientConfig.Headers)
			f.logClient = newGrpcClient(plogotlp.NewGRPCClient(grpcConn), grpcConn, logClientConfig, logMeta)
		}
	} else {
		logger.Info(f.context.GetRuntimeContext(), "otlp logs flusher", "disabled")
	}

	if metricErr == nil {
		if reflect.DeepEqual(f.GrpcConfig, metricClientConfig) && f.grpcConn != nil {
			logger.Info(f.context.GetRuntimeContext(), "otlp metrics flusher endpoint", f.GrpcConfig.Endpoint)
			f.metricClient = newGrpcClient(pmetricotlp.NewGRPCClient(f.grpcConn), f.grpcConn, metricClientConfig, f.metadata)
		} else {
			grpcConn, err := buildGrpcClientConn(metricClientConfig)
			if err != nil {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init otlp metric gRPC conn fail, error", err)
			}
			logger.Info(f.context.GetRuntimeContext(), "otlp metrics flusher endpoint", metricClientConfig.Endpoint)
			metricMeta := metadata.New(logClientConfig.Headers)
			f.metricClient = newGrpcClient(pmetricotlp.NewGRPCClient(grpcConn), grpcConn, metricClientConfig, metricMeta)
		}
	} else {
		logger.Info(f.context.GetRuntimeContext(), "otlp metrics flusher", "disabled")
	}

	if traceErr == nil {
		if reflect.DeepEqual(f.GrpcConfig, traceClientConfig) && f.grpcConn != nil {
			logger.Info(f.context.GetRuntimeContext(), "otlp traces flusher endpoint", f.GrpcConfig.Endpoint)
			f.traceClient = newGrpcClient(ptraceotlp.NewGRPCClient(f.grpcConn), f.grpcConn, traceClientConfig, f.metadata)
		} else {
			grpcConn, err := buildGrpcClientConn(traceClientConfig)
			if err != nil {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init otlp trace gRPC conn fail, error", err)
			}
			logger.Info(f.context.GetRuntimeContext(), "otlp traces flusher endpoint", traceClientConfig.Endpoint)
			traceMeta := metadata.New(logClientConfig.Headers)
			f.traceClient = newGrpcClient(ptraceotlp.NewGRPCClient(grpcConn), grpcConn, metricClientConfig, traceMeta)
		}
	} else {
		logger.Info(f.context.GetRuntimeContext(), "otlp traces flusher", "disabled")
	}

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
	if f.logClient != nil || f.logClient.grpcConn != nil {
		return nil
	}
	request := f.convertLogGroupToRequest(logGroupList)
	if !f.GrpcConfig.Retry.Enable {
		return timeoutFlush[plogotlp.ExportRequest, plogotlp.ExportResponse](f.logClient.client, request, f.metadata, f.logClient.grpcConfig)
	}
	return flushWithRetry[plogotlp.ExportRequest, plogotlp.ExportResponse](f.logClient.client, request, f.metadata, f.logClient.grpcConfig)
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

func (f *FlusherOTLP) SetUrgent(flag bool) {
}

// IsReady is ready to flush
func (f *FlusherOTLP) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	if f.logClient.grpcConn == nil || f.Logs.Disable {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_READY_ALARM", "otlp flusher is not ready, gRPC conn is nil")
		return false
	}
	state := f.logClient.grpcConn.GetState()
	var ready bool
	switch f.logClient.grpcConn.GetState() {
	case connectivity.Ready:
		ready = true
	case connectivity.Connecting:
		if !f.GrpcConfig.WaitForReady {
			ready = false
			break
		}
		timeout, canal := context.WithTimeout(context.Background(), f.logClient.grpcConfig.GetTimeout())
		defer canal()
		_ = f.logClient.grpcConn.WaitForStateChange(timeout, state)
		state = f.logClient.grpcConn.GetState()
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

// Stop ...
func (f *FlusherOTLP) Stop() error {
	var err error
	if f.grpcConn != nil {
		err = f.grpcConn.Close()
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_STOP_ALARM", "stop otlp flusher fail, error", err)
		}
	}

	if f.logClient != nil && f.logClient.grpcConn != nil {
		err = f.logClient.grpcConn.Close()
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_STOP_ALARM", "stop otlp logs flusher fail, error", err)
		}
	}

	if f.metricClient != nil && f.metricClient.grpcConn != nil {
		err = f.metricClient.grpcConn.Close()
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_STOP_ALARM", "stop otlp metrics flusher fail, error", err)
		}
	}

	if f.traceClient != nil && f.traceClient.grpcConn != nil {
		err = f.traceClient.grpcConn.Close()
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_STOP_ALARM", "stop otlp traces flusher fail, error", err)
		}
	}

	return err
}

func (f *FlusherOTLP) reConnect() connectivity.State {
	f.logClient.grpcConn.Connect()
	return f.logClient.grpcConn.GetState()
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

	if f.logClient != nil && f.logClient.grpcConn != nil {
		wg.Add(1)
		go func() {
			defer wg.Done()
			err := timeoutFlush[plogotlp.ExportRequest, plogotlp.ExportResponse](f.logClient.client, log, f.metadata, f.logClient.grpcConfig)
			if err != nil {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send log data to otlp server fail, error", err)
			}
			errChan <- err
		}()
	}

	if f.metricClient != nil && f.metricClient.grpcConn != nil {
		wg.Add(1)
		go func() {
			defer wg.Done()
			err := timeoutFlush[pmetricotlp.ExportRequest, pmetricotlp.ExportResponse](f.metricClient.client, metric, f.metadata, f.metricClient.grpcConfig)
			if err != nil {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send metric data to otlp server fail, error", err)
			}
			errChan <- err
		}()
	}

	if f.traceClient != nil && f.traceClient.grpcConn != nil {
		wg.Add(1)
		go func() {
			defer wg.Done()
			err := timeoutFlush[ptraceotlp.ExportRequest, ptraceotlp.ExportResponse](f.traceClient.client, trace, f.metadata, f.traceClient.grpcConfig)
			if err != nil {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send trace data to otlp server fail, error", err)
			}
			errChan <- err
		}()
	}

	wg.Wait()
	return <-errChan
}

func (f *FlusherOTLP) flushRequestsWithRetry(log plogotlp.ExportRequest, metric pmetricotlp.ExportRequest, trace ptraceotlp.ExportRequest) error {
	wg := sync.WaitGroup{}
	errChan := make(chan error, 3)

	if f.logClient != nil && f.logClient.grpcConn != nil {
		wg.Add(1)
		go func() {
			defer wg.Done()
			err := flushWithRetry[plogotlp.ExportRequest, plogotlp.ExportResponse](f.logClient.client, log, f.metadata, f.logClient.grpcConfig)
			if err != nil {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send log data to otlp server fail, error", err)
			}
			errChan <- err
		}()
	}

	if f.metricClient != nil && f.metricClient.grpcConn != nil {
		wg.Add(1)
		go func() {
			defer wg.Done()
			err := flushWithRetry[pmetricotlp.ExportRequest, pmetricotlp.ExportResponse](f.metricClient.client, metric, f.metadata, f.metricClient.grpcConfig)
			if err != nil {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send metric data to otlp server fail, error", err)
			}
			errChan <- err
		}()
	}

	if f.traceClient != nil && f.traceClient.grpcConn != nil {
		wg.Add(1)
		go func() {
			defer wg.Done()
			err := flushWithRetry[ptraceotlp.ExportRequest, ptraceotlp.ExportResponse](f.traceClient.client, trace, f.metadata, f.traceClient.grpcConfig)
			if err != nil {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "send trace data to otlp server fail, error", err)
			}
			errChan <- err
		}()
	}

	wg.Wait()
	return <-errChan
}

func flushWithRetry[
	T plogotlp.ExportRequest | pmetricotlp.ExportRequest | ptraceotlp.ExportRequest,
	P plogotlp.ExportResponse | pmetricotlp.ExportResponse | ptraceotlp.ExportResponse,
	Q interface {
		Export(ctx context.Context, request T, opts ...grpc.CallOption) (P, error)
	},
](q Q, request T, metadata metadata.MD, grpcConfig *helper.GrpcClientConfig) error {
	var retryNum = 0

	for {
		err := timeoutFlush[T, P](q, request, metadata, grpcConfig)
		if err == nil {
			return nil
		}
		retry := helper.GetRetryInfo(err)
		if retry == nil || retryNum >= grpcConfig.Retry.MaxCount {
			return err
		}

		retryNum++
		<-time.After(retry.ShouldDelay(grpcConfig.Retry.DefaultDelay))
	}
}

func timeoutFlush[
	T plogotlp.ExportRequest | pmetricotlp.ExportRequest | ptraceotlp.ExportRequest,
	P plogotlp.ExportResponse | pmetricotlp.ExportResponse | ptraceotlp.ExportResponse,
	Q interface {
		Export(ctx context.Context, request T, opts ...grpc.CallOption) (P, error)
	},
](
	q Q, req T, metadata metadata.MD, grpcConfig *helper.GrpcClientConfig) error {
	timeout, canal := context.WithTimeout(withMetadata(metadata), grpcConfig.GetTimeout())
	defer canal()
	_, err := q.Export(timeout, req)
	return err

}

// append withMetadata to context. refer to https://github.com/open-telemetry/opentelemetry-collector/blob/main/exporter/otlpexporter/otlp.go#L121
func withMetadata(md metadata.MD) context.Context {
	if md.Len() > 0 {
		return metadata.NewOutgoingContext(context.Background(), md)
	}
	return context.Background()
}

type grpcClient[T any] struct {
	client     T
	grpcConn   *grpc.ClientConn
	grpcConfig *helper.GrpcClientConfig
	metadata   metadata.MD
}

func newGrpcClient[T any](t T, conn *grpc.ClientConn, config *helper.GrpcClientConfig, metadata metadata.MD) *grpcClient[T] {
	return &grpcClient[T]{
		client:     t,
		grpcConn:   conn,
		grpcConfig: config,
		metadata:   metadata,
	}
}

func mergeGrpcConfig(defaultConfig *helper.GrpcClientConfig, specificConfig *GrpcClientConfig) (*helper.GrpcClientConfig, error) {
	if defaultConfig == nil && specificConfig == nil {
		return nil, errEmptyConfig
	}

	if specificConfig != nil && specificConfig.Disable {
		return nil, errEmptyConfig
	}

	// only has default config
	if specificConfig == nil {
		if defaultConfig.Endpoint == "" {
			return defaultConfig, errNoEndpoint
		}
		specificConfig = &GrpcClientConfig{
			GrpcClientConfig: *defaultConfig,
		}
		return &specificConfig.GrpcClientConfig, nil
	}

	// only has specific config
	if defaultConfig == nil {
		if specificConfig.Endpoint == "" {
			return &specificConfig.GrpcClientConfig, errNoEndpoint
		}
		return &specificConfig.GrpcClientConfig, nil
	}

	// has both configs
	if specificConfig.Endpoint == "" {
		specificConfig.Endpoint = defaultConfig.Endpoint
	}

	return &specificConfig.GrpcClientConfig, nil
}

func buildGrpcClientConn(grpcConfig *helper.GrpcClientConfig) (*grpc.ClientConn, error) {
	dialOpts, err := grpcConfig.GetDialOptions()
	if err != nil {
		return nil, err
	}

	return grpc.Dial(grpcConfig.GetEndpoint(), dialOpts...)
}

type GrpcClientConfig struct {
	helper.GrpcClientConfig
	Disable bool
}

func init() {
	pipeline.Flushers["flusher_otlp"] = func() pipeline.Flusher {
		return &FlusherOTLP{Version: v1}
	}
}
