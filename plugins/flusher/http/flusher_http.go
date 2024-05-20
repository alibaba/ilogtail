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

package http

import (
	"bytes"
	"compress/gzip"
	"crypto/rand"
	"errors"
	"fmt"
	"io"
	"math/big"
	"net/http"
	"net/url"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"github.com/golang/snappy"
	"k8s.io/apimachinery/pkg/util/net"

	"github.com/alibaba/ilogtail/pkg/fmtstr"
	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/pipeline/extensions"
	"github.com/alibaba/ilogtail/pkg/protocol"
	converter "github.com/alibaba/ilogtail/pkg/protocol/converter"
)

const (
	defaultTimeout = time.Minute

	contentTypeHeader     = "Content-Type"
	defaultContentType    = "application/octet-stream"
	contentEncodingHeader = "Content-Encoding"
)

var contentTypeMaps = map[string]string{
	converter.EncodingJSON:     "application/json",
	converter.EncodingProtobuf: defaultContentType,
	converter.EncodingNone:     defaultContentType,
	converter.EncodingCustom:   defaultContentType,
}

var supportedCompressionType = map[string]any{
	"gzip":   nil,
	"snappy": nil,
}

var (
	flusherID       = int64(0) // http flusher id that starts from 0
	sensitiveLabels = []string{"u", "user", "username", "p", "password", "passwd", "pwd", "Authorization"}
)

type retryConfig struct {
	Enable        bool          // If enable retry, default is true
	MaxRetryTimes int           // Max retry times, default is 3
	InitialDelay  time.Duration // Delay time before the first retry, default is 1s
	MaxDelay      time.Duration // max delay time when retry, default is 30s
}

type FlusherHTTP struct {
	RemoteURL              string                       // RemoteURL to request
	Headers                map[string]string            // Headers to append to the http request
	Query                  map[string]string            // Query parameters to append to the http request
	Timeout                time.Duration                // Request timeout, default is 60s
	Retry                  retryConfig                  // Retry strategy, default is retry 3 times with delay time begin from 1second, max to 30 seconds
	Convert                helper.ConvertConfig         // Convert defines which protocol and format to convert to
	Concurrency            int                          // How many requests can be performed in concurrent
	Authenticator          *extensions.ExtensionConfig  // name and options of the extensions.ClientAuthenticator extension to use
	FlushInterceptor       *extensions.ExtensionConfig  // name and options of the extensions.FlushInterceptor extension to use
	AsyncIntercept         bool                         // intercept the event asynchronously
	RequestInterceptors    []extensions.ExtensionConfig // custom request interceptor settings
	QueueCapacity          int                          // capacity of channel
	DropEventWhenQueueFull bool                         // If true, pipeline events will be dropped when the queue is full
	DebugMetrics           []string
	JitterInSec            int
	Compression            string

	varKeys []string

	context     pipeline.Context
	converter   *converter.Converter
	client      *http.Client
	interceptor extensions.FlushInterceptor

	broadcaster helper.Broadcaster
	queue       chan *groupEventsWithTimestamp
	counter     sync.WaitGroup

	matchedEvents        pipeline.Counter
	unmatchedEvents      pipeline.Counter
	droppedEvents        pipeline.Counter
	retryCount           pipeline.Counter
	flushFailure         pipeline.Counter
	flushLatency         pipeline.Counter
	statusCodeStatistics pipeline.MetricVector[pipeline.Counter]
}

type groupEventsWithTimestamp struct {
	data        any
	enqueueTime time.Time
}

func (f *FlusherHTTP) Description() string {
	return "http flusher for ilogtail"
}

func (f *FlusherHTTP) Init(context pipeline.Context) error {
	f.context = context
	logger.Info(f.context.GetRuntimeContext(), "http flusher init", "initializing")
	if f.RemoteURL == "" {
		err := errors.New("remoteURL is empty")
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher init fail, error", err)
		return err
	}

	if f.Concurrency < 1 {
		err := errors.New("concurrency must be greater than zero")
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher check concurrency fail, error", err)
		return err
	}

	converter, err := f.getConverter()
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher init converter fail, error", err)
		return err
	}
	f.converter = converter

	if f.FlushInterceptor != nil {
		var ext pipeline.Extension
		ext, err = f.context.GetExtension(f.FlushInterceptor.Type, f.FlushInterceptor.Options)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher init filter fail, error", err)
			return err
		}
		interceptor, ok := ext.(extensions.FlushInterceptor)
		if !ok {
			err = fmt.Errorf("filter(%s) not implement interface extensions.FlushInterceptor", f.FlushInterceptor)
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher init filter fail, error", err)
			return err
		}
		f.interceptor = interceptor
	}

	err = f.initHTTPClient()
	if err != nil {
		return err
	}

	if f.QueueCapacity <= 0 {
		f.QueueCapacity = 1024
	}
	f.queue = make(chan *groupEventsWithTimestamp, f.QueueCapacity)

	if f.JitterInSec > 0 {
		f.broadcaster = helper.NewBroadcaster(f.Concurrency)
	}

	for i := 0; i < f.Concurrency; i++ {
		ch := make(chan interface{}, 1)
		if f.JitterInSec > 0 {
			f.broadcaster.Register(ch)
		}
		go f.runFlushTask(ch)
	}

	f.buildVarKeys()
	f.fillRequestContentType()

	metricsRecord := f.context.GetMetricRecord()
	metricLabels := f.buildLabels()
	f.matchedEvents = helper.NewCounterMetricAndRegister(metricsRecord, "http_flusher_matched_events", metricLabels...)
	f.unmatchedEvents = helper.NewCounterMetricAndRegister(metricsRecord, "http_flusher_unmatched_events", metricLabels...)
	f.droppedEvents = helper.NewCounterMetricAndRegister(metricsRecord, "http_flusher_dropped_events", metricLabels...)
	f.retryCount = helper.NewCounterMetricAndRegister(metricsRecord, "http_flusher_retry_count", metricLabels...)
	f.flushFailure = helper.NewCounterMetricAndRegister(metricsRecord, "http_flusher_flush_failure_count", metricLabels...)
	f.flushLatency = helper.NewAverageMetricAndRegister(metricsRecord, "http_flusher_flush_latency_ns", metricLabels...)

	f.statusCodeStatistics = helper.NewCounterMetricVectorAndRegister(metricsRecord,
		"http_flusher_status_code_count",
		helper.LogContentsToMap(metricLabels),
		[]string{"status_code"})

	logger.Info(f.context.GetRuntimeContext(), "http flusher init", "initialized",
		"timeout", f.Timeout,
		"jitter", time.Duration(f.JitterInSec)*time.Second,
		"compression", f.Compression,
		"debug metrics", f.DebugMetrics)
	return nil
}

func (f *FlusherHTTP) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	now := time.Now()
	for _, logGroup := range logGroupList {
		f.addTask(logGroup, now)
	}
	return nil
}

func (f *FlusherHTTP) Export(groupEventsArray []*models.PipelineGroupEvents, ctx pipeline.PipelineContext) error {
	now := time.Now()
	for _, groupEvents := range groupEventsArray {
		if groupEvents == nil {
			continue
		}

		if !f.AsyncIntercept && f.interceptor != nil {
			originCount := int64(len(groupEvents.Events))
			groupEvents = f.interceptor.Intercept(groupEvents)
			f.unmatchedEvents.Add(getInterceptedEventCount(originCount, groupEvents))
			// skip groupEvents that is nil or empty.
			if groupEvents == nil || len(groupEvents.Events) == 0 {
				continue
			}
		}
		f.addTask(groupEvents, now)
	}
	return nil
}

func (f *FlusherHTTP) SetUrgent(flag bool) {
}

func (f *FlusherHTTP) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	return f.client != nil
}

func (f *FlusherHTTP) Stop() error {
	if f.broadcaster != nil {
		f.broadcaster.Close() // close the broadcaster to flush all the data in the queue
	}

	f.counter.Wait()
	close(f.queue)
	return nil
}

func (f *FlusherHTTP) initHTTPClient() error {
	transport := http.DefaultTransport
	if dt, ok := transport.(*http.Transport); ok {
		dt = dt.Clone()
		if f.Concurrency > dt.MaxIdleConnsPerHost {
			dt.MaxIdleConnsPerHost = f.Concurrency + 1
		}
		transport = dt
	}

	var err error
	transport, err = f.initRequestInterceptors(transport)
	if err != nil {
		return err
	}

	if f.Authenticator != nil {
		var auth pipeline.Extension
		auth, err = f.context.GetExtension(f.Authenticator.Type, f.Authenticator.Options)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher init authenticator fail, error", err)
			return err
		}
		ca, ok := auth.(extensions.ClientAuthenticator)
		if !ok {
			err = fmt.Errorf("authenticator(%s) not implement interface extensions.ClientAuthenticator", f.Authenticator)
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher init authenticator fail, error", err)
			return err
		}
		transport, err = ca.RoundTripper(transport)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher init authenticator fail, error", err)
			return err
		}
	}

	f.client = &http.Client{
		Timeout:   f.Timeout,
		Transport: transport,
	}
	return nil
}

func (f *FlusherHTTP) initRequestInterceptors(transport http.RoundTripper) (http.RoundTripper, error) {
	for i := len(f.RequestInterceptors) - 1; i >= 0; i-- {
		setting := f.RequestInterceptors[i]
		ext, err := f.context.GetExtension(setting.Type, setting.Options)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher init request interceptor fail, error", err)
			return nil, err
		}
		interceptor, ok := ext.(extensions.RequestInterceptor)
		if !ok {
			err = fmt.Errorf("interceptor(%s) with type %T not implement interface extensions.RequestInterceptor", setting.Type, ext)
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher init request interceptor fail, error", err)
			return nil, err
		}
		transport, err = interceptor.RoundTripper(transport)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher init request interceptor fail, error", err)
			return nil, err
		}
	}
	return transport, nil
}

func (f *FlusherHTTP) getConverter() (*converter.Converter, error) {
	return converter.NewConverterWithSep(f.Convert.Protocol, f.Convert.Encoding, f.Convert.Separator, f.Convert.IgnoreUnExpectedData, f.Convert.TagFieldsRename, f.Convert.ProtocolFieldsRename, f.context.GetPipelineScopeConfig())
}

func (f *FlusherHTTP) addTask(log interface{}, enqueueTime time.Time) {
	f.counter.Add(1)

	// if queue is under high pressure, wake up all flushTask.
	if f.JitterInSec > 0 && len(f.queue) >= cap(f.queue)/2 {
		f.broadcaster.TrySubmit(nil)
	}

	evts := &groupEventsWithTimestamp{
		data:        log,
		enqueueTime: enqueueTime,
	}

	if f.DropEventWhenQueueFull {
		select {
		case f.queue <- evts:
		default:
			f.handleDroppedEvent(log)
		}
	} else {
		f.queue <- evts
	}
}

// handleDroppedEvent handles a dropped event and reports metrics.
func (f *FlusherHTTP) handleDroppedEvent(log interface{}) {
	f.counter.Done()

	// Update the dropped events counter based on the type of the log.
	switch v := log.(type) {
	case *protocol.LogGroup:
		if v != nil {
			f.droppedEvents.Add(int64(len(v.Logs)))
		}
	case *models.PipelineGroupEvents:
		if v != nil {
			f.droppedEvents.Add(int64(len(v.Events)))
		}
	}
	logger.Warningf(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher dropped a group event since the queue is full")
}

func (f *FlusherHTTP) countDownTask() {
	f.counter.Done()
}

func (f *FlusherHTTP) runFlushTask(broadcastCh chan interface{}) {
	jitterDuration := time.Duration(f.JitterInSec) * time.Second

	for event := range f.queue {
		eventWaitTime := time.Since(event.enqueueTime)

		if eventWaitTime < jitterDuration {
			if len(f.queue) < f.Concurrency {
				maxSleepDuration := jitterDuration - eventWaitTime
				randomSleep(maxSleepDuration, broadcastCh)
			}
		}

		err := f.convertAndFlush(event.data)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher failed convert or flush data, data dropped, error", err)
		}
	}
}

func (f *FlusherHTTP) convertAndFlush(data interface{}) error {
	defer f.countDownTask()
	var logs interface{}
	var varValues []map[string]string
	var err error
	switch v := data.(type) {
	case *protocol.LogGroup:
		logs, varValues, err = f.converter.ToByteStreamWithSelectedFields(v, f.varKeys)
	case *models.PipelineGroupEvents:
		if f.AsyncIntercept && f.interceptor != nil {
			originCount := int64(len(v.Events))
			v = f.interceptor.Intercept(v)
			f.unmatchedEvents.Add(getInterceptedEventCount(originCount, v))
			if v == nil || len(v.Events) == 0 {
				return nil
			}
		}
		f.matchedEvents.Add(int64(len(v.Events)))
		if len(f.DebugMetrics) > 0 {
			for _, target := range f.DebugMetrics {
				for _, m := range v.Events {
					if m.GetName() == target {
						logger.Infof(f.context.GetRuntimeContext(), "http flusher found target metric: %s: details: %v", target, m.(*models.Metric).String())
					}
				}
			}
		}
		logs, varValues, err = f.converter.ToByteStreamWithSelectedFieldsV2(v, f.varKeys)
	default:
		return fmt.Errorf("unsupport data type")
	}

	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher converter log fail, error", err)
		return err
	}

	switch rows := logs.(type) {
	case [][]byte:
		for idx, data := range rows {
			body, values := data, varValues[idx]
			err = f.flushWithRetry(body, values)
			if err != nil {
				f.flushFailure.Add(1)
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher failed flush data after retry, data dropped, error", err,
					"remote url", f.RemoteURL, "headers", f.getInsensitiveMap(f.Headers), "query", f.getInsensitiveMap(f.Query))
			}
		}
		return nil
	case []byte:
		err = f.flushWithRetry(rows, nil)
		if err != nil {
			f.flushFailure.Add(1)
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher failed flush data after retry, data dropped, error", err,
				"remote url", f.RemoteURL, "headers", f.getInsensitiveMap(f.Headers), "query", f.getInsensitiveMap(f.Query))
		}
		return err
	default:
		err = fmt.Errorf("not supported logs type [%T]", logs)
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher failed flush data, error", err)
		return err
	}
}

func (f *FlusherHTTP) flushWithRetry(data []byte, varValues map[string]string) error {
	var err error
	start := time.Now()

	for i := 0; i <= f.Retry.MaxRetryTimes; i++ {
		if i > 0 { // first flush is not retry
			f.retryCount.Add(1)
		}

		ok, retryable, e := f.flush(data, varValues)
		isEoF := isErrorEOF(e)
		//  retry if the error is io.EOF.
		if ok || (!retryable && !isEoF) || !f.Retry.Enable {
			err = e
			break
		}

		if isEoF {
			logger.Debugf(f.context.GetRuntimeContext(), "http flusher sent requests and got EOF, will retry")
		}

		err = e
		<-time.After(f.getNextRetryDelay(i))
	}
	converter.PutPooledByteBuf(&data)
	f.flushLatency.Add(time.Since(start).Nanoseconds())
	return err
}

func (f *FlusherHTTP) getNextRetryDelay(retryTime int) time.Duration {
	delay := f.Retry.InitialDelay * 1 << time.Duration(retryTime)
	if delay > f.Retry.MaxDelay {
		delay = f.Retry.MaxDelay
	}

	// apply about equaly distributed jitter in second half of the interval, such that the wait
	// time falls into the interval [dur/2, dur]
	harf := int64(delay / 2)
	jitter, err := rand.Int(rand.Reader, big.NewInt(harf+1))
	if err != nil {
		return delay
	}
	return time.Duration(harf + jitter.Int64())
}

func (f *FlusherHTTP) compressData(data []byte) (io.Reader, error) {
	var reader io.Reader = bytes.NewReader(data)
	if compressionType, ok := f.Headers[contentEncodingHeader]; ok {
		switch compressionType {
		case "gzip":
			var buf bytes.Buffer
			gw := gzip.NewWriter(&buf)
			if _, err := gw.Write(data); err != nil {
				return nil, err
			}
			if err := gw.Close(); err != nil {
				return nil, err
			}
			reader = &buf
		case "snappy":
			compressedData := snappy.Encode(nil, data)
			reader = bytes.NewReader(compressedData)
		default:
		}
	}
	return reader, nil
}

func (f *FlusherHTTP) flush(data []byte, varValues map[string]string) (ok, retryable bool, err error) {
	reader, err := f.compressData(data)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "create reader error", err)
		return false, false, err
	}

	req, err := http.NewRequest(http.MethodPost, f.RemoteURL, reader)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher create request fail, error", err)
		return false, false, err
	}

	if len(f.Query) > 0 {
		values := req.URL.Query()
		for k, v := range f.Query {
			if len(f.varKeys) == 0 {
				values.Add(k, v)
				continue
			}

			fv, ferr := fmtstr.FormatTopic(varValues, v)
			if ferr != nil {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher format query fail, error", ferr)
			} else {
				v = *fv
			}
			values.Add(k, v)
		}
		req.URL.RawQuery = values.Encode()
	}

	for k, v := range f.Headers {
		if len(f.varKeys) == 0 {
			req.Header.Add(k, v)
			continue
		}
		fv, ferr := fmtstr.FormatTopic(varValues, v)
		if ferr != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher format header fail, error", ferr)
		} else {
			v = *fv
		}
		req.Header.Add(k, v)
	}

	response, err := f.client.Do(req)
	if logger.DebugFlag() {
		logger.Debugf(f.context.GetRuntimeContext(), "request [method]: %v; [header]: %v; [url]: %v; [body]: %v", req.Method, req.Header, req.URL, string(data))
	}
	if err != nil {
		urlErr, ok := err.(*url.Error)
		retry := false
		if ok && (urlErr.Timeout() || urlErr.Temporary()) {
			retry = true
		}
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALRAM", "http flusher send request fail, error", err)
		return false, retry, err
	}
	body, err := io.ReadAll(response.Body)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALRAM", "http flusher read response fail, error", err)
		return false, false, err
	}
	err = response.Body.Close()
	if err != nil {
		logger.Warning(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher close response body fail, error", err)
		return false, false, err
	}

	_ = f.statusCodeStatistics.WithLabels(pipeline.Label{Key: "status_code", Value: strconv.Itoa(response.StatusCode)}).Add(1)

	switch response.StatusCode / 100 {
	case 2:
		return true, false, nil
	case 5:
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher write data returned error, url", req.URL.String(), "status", response.Status, "body", string(body))
		return false, true, fmt.Errorf("err status returned: %v", response.Status)
	default:
		if response.StatusCode == http.StatusUnauthorized || response.StatusCode == http.StatusForbidden {
			return false, true, fmt.Errorf("err status returned: %v", response.Status)
		}

		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher write data returned error, url", req.URL.String(), "status", response.Status, "body", string(body))
		return false, false, fmt.Errorf("unexpected status returned: %v", response.Status)
	}
}

func (f *FlusherHTTP) buildLabels() []*protocol.Log_Content {
	id := atomic.AddInt64(&flusherID, 1) - 1
	labels := make([]*protocol.Log_Content, 0, len(f.Query)+2)
	labels = append(labels, &protocol.Log_Content{Key: "RemoteURL", Value: f.RemoteURL})
	for k, v := range f.Query {
		if !isSensitiveKey(k) {
			labels = append(labels, &protocol.Log_Content{Key: k, Value: v})
		}
	}
	labels = append(labels, &protocol.Log_Content{Key: "flusher_http_id", Value: strconv.FormatInt(id, 10)})
	return labels
}

func (f *FlusherHTTP) getInsensitiveMap(info map[string]string) map[string]string {
	res := make(map[string]string, len(info))
	for k, v := range info {
		if !isSensitiveKey(k) {
			res[k] = v
		}
	}
	return res
}

func isSensitiveKey(label string) bool {
	for _, sensitiveKey := range sensitiveLabels {
		if strings.ToLower(label) == sensitiveKey {
			return true
		}
	}
	return false
}

func (f *FlusherHTTP) buildVarKeys() {
	cache := map[string]struct{}{}
	defines := []map[string]string{f.Query, f.Headers}

	for _, define := range defines {
		for _, v := range define {
			keys, err := fmtstr.CompileKeys(v)
			if err != nil {
				logger.Warning(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher init varKeys fail, err", err)
			}
			for _, key := range keys {
				cache[key] = struct{}{}
			}
		}
	}

	varKeys := make([]string, 0, len(cache))
	for k := range cache {
		varKeys = append(varKeys, k)
	}
	f.varKeys = varKeys
}

func (f *FlusherHTTP) fillRequestContentType() {
	if f.Headers == nil {
		f.Headers = make(map[string]string, 4)
	}

	if f.Compression != "" {
		if _, ok := supportedCompressionType[f.Compression]; ok {
			f.Headers[contentEncodingHeader] = f.Compression
		}
	}

	_, ok := f.Headers[contentTypeHeader]
	if ok {
		return
	}

	contentType, ok := contentTypeMaps[f.Convert.Encoding]
	if !ok {
		contentType = defaultContentType
	}
	f.Headers[contentTypeHeader] = contentType
}

func getInterceptedEventCount(origin int64, group *models.PipelineGroupEvents) int64 {
	if group == nil {
		return origin
	}
	return origin - int64(len(group.Events))
}

func isErrorEOF(err error) bool {
	return errors.Is(err, io.EOF) || net.IsProbableEOF(err)
}

// randomSleep sleeps for a random duration between 0 and jitterInSec.
// If jitterInSec is 0, it will skip sleep.
// If shutdown is closed, it will stop sleep immediately.
func randomSleep(maxJitter time.Duration, stopChan chan interface{}) {
	if maxJitter == 0 {
		return
	}

	sleepTime := getJitter(maxJitter)
	t := time.NewTimer(sleepTime)
	select {
	case <-t.C:
		return
	case <-stopChan:
		t.Stop()
		return
	}
}

func getJitter(maxJitter time.Duration) time.Duration {
	jitter, err := rand.Int(rand.Reader, big.NewInt(int64(maxJitter)))
	if err != nil {
		return 0
	}
	return time.Duration(jitter.Int64())
}

func init() {
	pipeline.Flushers["flusher_http"] = func() pipeline.Flusher {
		return &FlusherHTTP{
			QueueCapacity: 1024,
			Timeout:       defaultTimeout,
			Concurrency:   1,
			JitterInSec:   0,
			Convert: helper.ConvertConfig{
				Protocol:             converter.ProtocolCustomSingle,
				Encoding:             converter.EncodingJSON,
				IgnoreUnExpectedData: true,
			},
			Retry: retryConfig{
				Enable:        true,
				MaxRetryTimes: 3,
				InitialDelay:  time.Second,
				MaxDelay:      30 * time.Second,
			},
			DropEventWhenQueueFull: true,
		}
	}
}
