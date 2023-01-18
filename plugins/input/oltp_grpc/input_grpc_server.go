// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      grpc://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package oltp_grpc

import (
	"net"
	"net/url"
	"strings"
	"sync"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/logger"
	"go.opentelemetry.io/collector/pdata/plog/plogotlp"
	"go.opentelemetry.io/collector/pdata/pmetric/pmetricotlp"
	"go.opentelemetry.io/collector/pdata/ptrace/ptraceotlp"
	"google.golang.org/grpc"
)

// ServerGRPC implements ServiceInputV2
// It can only work in v2 pipelines.
type ServerGRPC struct {
	context         ilogtail.Context
	piplineContext  ilogtail.PipelineContext
	server          *grpc.Server
	listener        net.Listener
	wg              sync.WaitGroup
	logsReceiver    plogotlp.GRPCServer // currently logs are not supported
	tracesReceiver  ptraceotlp.GRPCServer
	metricsReceiver pmetricotlp.GRPCServer

	Address              string
	MaxRecvMsgSizeMiB    int
	MaxConcurrentStreams int
	ReadBufferSize       int
	WriteBufferSize      int
	FieldsExtend         bool
}

// Init ...
func (s *ServerGRPC) Init(context ilogtail.Context) (int, error) {
	s.context = context
	logger.Info(s.context.GetRuntimeContext(), "oltp grpc server init", "initializing")
	server := grpc.NewServer(
		s.serverOptions()...,
	)
	s.server = server
	logger.Info(s.context.GetRuntimeContext(), "oltp grpc server init", "initialized")
	return 0, nil
}

// Description ...
func (s *ServerGRPC) Description() string {
	return "Open-Telemetry GRPC service input plugin for logtail"
}

// StartService(PipelineContext) error
func (s *ServerGRPC) StartService(ctx ilogtail.PipelineContext) error {
	s.piplineContext = ctx
	s.wg.Add(1)

	listener, err := getNetListener(s.Address)
	if err != nil {
		return err
	}
	s.listener = listener

	s.tracesReceiver = newTracesReceiver(ctx)
	ptraceotlp.RegisterGRPCServer(s.server, s.tracesReceiver)
	logger.Info(s.context.GetRuntimeContext(), "oltp traces receiver", "initialized")

	s.metricsReceiver = newMetricsReceiver(ctx)
	pmetricotlp.RegisterGRPCServer(s.server, s.metricsReceiver)
	logger.Info(s.context.GetRuntimeContext(), "oltp metrics receiver", "initialized")

	s.logsReceiver = newLogsReceiver(ctx)
	plogotlp.RegisterGRPCServer(s.server, s.logsReceiver)
	logger.Info(s.context.GetRuntimeContext(), "oltp logs receiver", "initialized")

	go func() {
		logger.Info(s.context.GetRuntimeContext(), "oltp grpc server start", s.Address)
		_ = s.server.Serve(listener)
		s.server.GracefulStop()
		logger.Info(s.context.GetRuntimeContext(), "oltp grpc server shutdown", s.Address)
		s.wg.Done()
	}()
	return nil
}

// Stop stops the services and closes any necessary channels and connections
func (s *ServerGRPC) Stop() error {
	if s.listener != nil {
		_ = s.listener.Close()
		logger.Info(s.context.GetRuntimeContext(), "oltp grpc server stop", s.Address)
		s.wg.Wait()
	}
	return nil
}

func (s *ServerGRPC) serverOptions() []grpc.ServerOption {
	var opts []grpc.ServerOption
	if s.MaxRecvMsgSizeMiB > 0 {
		opts = append(opts, grpc.MaxRecvMsgSize(int(s.MaxRecvMsgSizeMiB*1024*1024)))
	}
	if s.MaxConcurrentStreams > 0 {
		opts = append(opts, grpc.MaxConcurrentStreams(uint32(s.MaxConcurrentStreams)))
	}

	if s.ReadBufferSize > 0 {
		opts = append(opts, grpc.ReadBufferSize(s.ReadBufferSize))
	}

	if s.WriteBufferSize > 0 {
		opts = append(opts, grpc.WriteBufferSize(s.WriteBufferSize))
	}
	return opts
}

func getNetListener(endpoint string) (net.Listener, error) {
	var listener net.Listener
	var err error
	switch {
	case strings.HasPrefix(endpoint, "http") ||
		strings.HasPrefix(endpoint, "https") ||
		strings.HasPrefix(endpoint, "tcp"):
		configURL, errAddr := url.Parse(endpoint)
		if errAddr != nil {
			return nil, errAddr
		}
		listener, err = net.Listen("tcp", configURL.Host)
	default:
		listener, err = net.Listen("tcp", endpoint)
	}
	return listener, err
}

func init() {
	ilogtail.ServiceInputs["service_oltp_grpc"] = func() ilogtail.ServiceInput {
		return &ServerGRPC{}
	}
}
