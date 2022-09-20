package grpchelper

import (
	"time"
)

type ClientConfig struct {
	Endpoint string

	// The compression key for supported compression types within collector.
	Compression string

	// The headers associated with gRPC requests.
	Headers map[string]string

	// Sets the balancer in grpclb_policy to discover the servers. Default is pick_first.
	// https://github.com/grpc/grpc-go/blob/master/examples/features/load_balancing/README.md
	BalancerName string

	// WaitForReady parameter configures client to wait for ready state before sending data.
	// (https://github.com/grpc/grpc/blob/master/doc/wait-for-ready.md)
	WaitForReady bool

	// ReadBufferSize for gRPC client. See grpchelper.WithReadBufferSize.
	// (https://godoc.org/google.golang.org/grpc#WithReadBufferSize).
	ReadBufferSize int

	// WriteBufferSize for gRPC gRPC. See grpchelper.WithWriteBufferSize.
	// (https://godoc.org/google.golang.org/grpc#WithWriteBufferSize).
	WriteBufferSize int

	// Send retry setting
	Retry RetryConfig `json:"retry"`
}

type RetryConfig struct {
	Enable       bool
	MaxCount     int           `json:"max_count"`
	DefaultDelay time.Duration `json:"default_delay"`
}
