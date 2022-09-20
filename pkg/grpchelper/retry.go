package grpchelper

import (
	"time"

	"google.golang.org/genproto/googleapis/rpc/errdetails"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

// RetryInfo Handle retry for grpc. Refer to https://github.com/open-telemetry/opentelemetry-collector/blob/main/exporter/otlpexporter/otlp.go#L121
type RetryInfo struct {
	delay time.Duration
	err   error
}

func (r *RetryInfo) Error() error {
	return r.err
}

func (r *RetryInfo) ShouldDelay(delay time.Duration) time.Duration {
	if r.delay != 0 {
		return r.delay
	}
	return delay
}

func GetRetryInfo(err error) *RetryInfo {
	if err == nil {
		// Request is successful, we are done.
		return nil
	}
	// We have an error, check gRPC status code.

	st := status.Convert(err)
	if st.Code() == codes.OK {
		// Not really an error, still success.
		return nil
	}
	retryInfo := getRetryInfo(st)

	if !shouldRetry(st.Code(), retryInfo) {
		// It is not a retryable error, we should not retry.
		return nil
	}
	throttle := getThrottleDuration(retryInfo)
	return &RetryInfo{delay: throttle, err: err}
}

func shouldRetry(code codes.Code, retryInfo *errdetails.RetryInfo) bool {
	switch code {
	case codes.Canceled,
		codes.DeadlineExceeded,
		codes.Aborted,
		codes.OutOfRange,
		codes.Unavailable,
		codes.DataLoss:
		// These are retryable errors.
		return true
	case codes.ResourceExhausted:
		// Retry only if RetryInfo was supplied by the server.
		// This indicates that the server can still recover from resource exhaustion.
		return retryInfo != nil
	}
	// Don't retry on any other code.
	return false
}

func getRetryInfo(status *status.Status) *errdetails.RetryInfo {
	for _, detail := range status.Details() {
		if t, ok := detail.(*errdetails.RetryInfo); ok {
			return t
		}
	}
	return nil
}

func getThrottleDuration(t *errdetails.RetryInfo) time.Duration {
	if t == nil || t.RetryDelay == nil {
		return 0
	}
	if t.RetryDelay.Seconds > 0 || t.RetryDelay.Nanos > 0 {
		return time.Duration(t.RetryDelay.Seconds)*time.Second + time.Duration(t.RetryDelay.Nanos)*time.Nanosecond
	}
	return 0
}
