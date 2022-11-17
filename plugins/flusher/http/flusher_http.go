package http

import (
	"bytes"
	"errors"
	"fmt"
	"io/ioutil"
	"net/http"
	"sync"
	"time"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/fmtstr"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
	converter "github.com/alibaba/ilogtail/pkg/protocol/converter"
)

const (
	defaultTimeout = time.Minute
)

type convertConfig struct {
	TagFieldsRename      map[string]string // Rename one or more fields from tags.
	ProtocolFieldsRename map[string]string // Rename one or more fields, The protocol field options can only be: contents, tags, time
	Protocol             string            // Convert protocol, default value: custom_single
	Encoding             string            // Convert encoding, options are: 'json','none', default value:json
}

type retryConfig struct {
	Enable   bool          // If enable retry, default is true
	MaxCount int           // Max retry times, default is 3
	Delay    time.Duration // Delay time before next retry, default is 100 ms
}

type FlusherHTTP struct {
	RemoteURL   string            // RemoteURL to request
	Headers     map[string]string // Headers to append to the http request
	Query       map[string]string // Query parameters to append to the http request
	Timeout     time.Duration     // Request timeout, default is 60s
	Retry       retryConfig       // Retry strategy, default is retry 3 times with 100 milliseconds each time
	Convert     convertConfig     // Convert defines which protocol and format to convert to
	Concurrency int               // How many requests can be performed in concurrent

	queryVarKeys []string

	context   ilogtail.Context
	converter *converter.Converter
	client    *http.Client

	tokenCh chan struct{}
	tokenWg sync.WaitGroup
	stopCh  chan struct{}
}

func (f *FlusherHTTP) Description() string {
	return "http flusher for ilogtail"
}

func (f *FlusherHTTP) Init(context ilogtail.Context) error {
	f.context = context
	logger.Info(f.context.GetRuntimeContext(), "http flusher init", "initializing")
	if f.RemoteURL == "" {
		err := errors.New("remoteURL is empty")
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher init fail, error", err)
		return err
	}

	converter, err := f.getConverter()
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher init converter fail, error", err)
		return err
	}
	f.converter = converter

	f.client = &http.Client{
		Timeout: f.Timeout,
	}

	if f.Concurrency < 1 {
		err := errors.New("Concurrency must be greater than zero")
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher check concurrency fail, error", err)
		return err
	}
	f.tokenCh = make(chan struct{}, f.Concurrency)
	f.stopCh = make(chan struct{}, 1)

	f.buildQueryVarKeys()

	logger.Info(f.context.GetRuntimeContext(), "http flusher init", "initialized")
	return nil
}

func (f *FlusherHTTP) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	for _, logGroup := range logGroupList {
		err := f.convertAndFlush(logGroup)
		if err != nil {
			return err
		}
	}
	return nil
}

func (f *FlusherHTTP) SetUrgent(flag bool) {
}

func (f *FlusherHTTP) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	return f.client != nil
}

func (f *FlusherHTTP) Stop() error {
	f.stopCh <- struct{}{}
	f.tokenWg.Wait()
	return nil
}

func (f *FlusherHTTP) getConverter() (*converter.Converter, error) {
	return converter.NewConverter(f.Convert.Protocol, f.Convert.Encoding, nil, nil)
}

func (f *FlusherHTTP) acquireToken() bool {
	select {
	case f.tokenCh <- struct{}{}:
		f.tokenWg.Add(1)
		return true
	case <-f.stopCh:
		return false
	}
}

func (f *FlusherHTTP) releaseToken() {
	f.tokenWg.Done()
	<-f.tokenCh
}

func (f *FlusherHTTP) convertAndFlush(logGroup *protocol.LogGroup) error {
	logs, varValues, err := f.converter.ToByteStreamWithSelectedFields(logGroup, f.queryVarKeys)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher converter log fail, error", err)
		return err
	}
	switch rows := logs.(type) {
	case [][]byte:
		for idx, data := range rows {
			body, values := data, varValues[idx]
			if !f.acquireToken() {
				break
			}
			go func() {
				defer f.releaseToken()
				for i := 0; i <= f.Retry.MaxCount; i++ {
					ok, retryable, _ := f.flush(body, values)
					if ok || !retryable || !f.Retry.Enable {
						break
					}
					<-time.After(f.Retry.Delay)
				}
				converter.PutPooledByteBuf(&body)
			}()
		}
		return nil
	default:
		err = fmt.Errorf("not supported logs type [%T]", logs)
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "error", err)
		return err
	}
}

func (f *FlusherHTTP) flush(data []byte, varValues map[string]string) (ok, retryable bool, err error) {
	req, err := http.NewRequest(http.MethodPost, f.RemoteURL, bytes.NewReader(data))
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher create request fail, error", err)
		return false, false, err
	}

	if len(f.Query) > 0 {
		values := req.URL.Query()
		for k, v := range f.Query {
			if len(f.queryVarKeys) == 0 {
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
		req.Header.Add(k, v)
	}
	response, err := f.client.Do(req)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALRAM", "http flusher send request fail, error", err)
		return false, false, err
	}
	body, err := ioutil.ReadAll(response.Body)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALRAM", "http flusher read response fail, error", err)
		return false, false, err
	}
	err = response.Body.Close()
	if err != nil {
		logger.Warning(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher close response body fail, error", err)
		return false, false, err
	}
	switch response.StatusCode / 100 {
	case 2:
		return true, false, nil
	case 5:
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher write data returned error, status", response.Status, "body", string(body))
		return false, true, fmt.Errorf("err status returned: %v", response.Status)
	default:
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher write data returned error, status", response.Status, "body", string(body))
		return false, false, fmt.Errorf("unexpected status returned: %v", response.Status)
	}
}

func (f *FlusherHTTP) buildQueryVarKeys() {
	var varKeys []string
	for _, v := range f.Query {
		keys, err := fmtstr.CompileKeys(v)
		if err != nil {
			logger.Warning(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "http flusher init queryVarKeys fail, err", err)
		}
		for _, key := range keys {
			var exists bool
			for _, k := range varKeys {
				if k == v {
					exists = true
					break
				}
			}
			if exists {
				continue
			}
			varKeys = append(varKeys, key)
		}
	}
	f.queryVarKeys = varKeys
}

func init() {
	ilogtail.Flushers["flusher_http"] = func() ilogtail.Flusher {
		return &FlusherHTTP{
			Timeout:     defaultTimeout,
			Concurrency: 1,
			Convert: convertConfig{
				Protocol: converter.ProtocolCustomSingle,
				Encoding: converter.EncodingJSON,
			},
			Retry: retryConfig{
				Enable:   true,
				MaxCount: 3,
				Delay:    100 * time.Microsecond,
			},
		}
	}
}
