package http

import (
	"bytes"
	"errors"
	"fmt"
	"io/ioutil"
	"net/http"
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
	// Rename one or more fields from tags.
	TagFieldsRename map[string]string
	// Rename one or more fields, The protocol field options can only be: contents, tags, time
	ProtocolFieldsRename map[string]string
	// Convert protocol, default value: custom_single
	Protocol string
	// Convert encoding, default value:json
	// The options are: 'json'
	Encoding string
}

type retryConfig struct {
	Enable       bool
	MaxCount     int
	DefaultDelay time.Duration
}

type FlusherHTTP struct {
	RemoteURL string
	Headers   map[string]string
	Query     map[string]string
	Timeout   time.Duration
	Retry     retryConfig
	Convert   convertConfig

	queryVarKeys []string

	context   ilogtail.Context
	converter *converter.Converter
	client    *http.Client
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

	f.buildQueryVarKeys()

	logger.Info(f.context.GetRuntimeContext(), "http flusher init", "initialized")
	return nil
}

func (f *FlusherHTTP) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	for _, logGroup := range logGroupList {
		logs, varValues, err := f.converter.ToByteStreamWithSelectedFields(logGroup, f.queryVarKeys)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "http flusher converter log fail, error", err)
			continue
		}
		switch rows := logs.(type) {
		case [][]byte:
			for idx, data := range rows {
				for i := 0; i <= f.Retry.MaxCount; i++ {
					ok, retryable, _ := f.flush(data, varValues[idx])
					if ok || !retryable || !f.Retry.Enable {
						break
					}
					<-time.After(f.Retry.DefaultDelay)
				}
			}
		default:
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "not supported logs type", fmt.Sprintf("%T", logs))
		}
	}
	return nil
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

func (f *FlusherHTTP) SetUrgent(flag bool) {
}

func (f *FlusherHTTP) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	return f.client != nil
}

func (f *FlusherHTTP) Stop() error {
	return nil
}

func (f *FlusherHTTP) getConverter() (*converter.Converter, error) {
	return converter.NewConverter(f.Convert.Protocol, f.Convert.Encoding, nil, nil)
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
			Timeout: defaultTimeout,
			Convert: convertConfig{
				Protocol: converter.ProtocolCustomSingle,
				Encoding: converter.EncodingJSON,
			},
		}
	}
}
