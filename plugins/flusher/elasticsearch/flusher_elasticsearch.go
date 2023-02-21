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

package elasticsearch

import (
	"bytes"
	"context"
	"crypto/tls"
	"fmt"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	converter "github.com/alibaba/ilogtail/pkg/protocol/converter"
	"github.com/elastic/go-elasticsearch/v8"
	"github.com/elastic/go-elasticsearch/v8/esapi"
	"net"
	"strconv"
	"time"
)

type FlusherElasticSearch struct {
	// Convert ilogtail data convert config
	Convert convertConfig
	// Addresses elasticsearch addresses
	Addresses []string
	// Authentication
	Authentication Authentication

	//Configuration of client's Transport
	MaxIdleConnsPerHost   int
	ResponseHeaderTimeout time.Duration
	DialContext           net.Conn
	//Tls min version
	MinVersion int

	context   pipeline.Context
	converter *converter.Converter
	esClient  *elasticsearch.Client
	flusher   FlusherFunc
}

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

type FlusherFunc func(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error

func NewFlusherElasticSearch() *FlusherElasticSearch {
	return &FlusherElasticSearch{
		Addresses: []string{},
		Authentication: Authentication{
			PlainText: &PlainTextConfig{
				Username: "",
				Password: "",
				Index:    "",
			},
			cacertFilePath: "",
		},
		MaxIdleConnsPerHost:   10,
		ResponseHeaderTimeout: time.Second,
		DialContext:           nil,
		MinVersion:            tls.VersionTLS12,
		Convert: convertConfig{
			Protocol: converter.ProtocolCustomSingle,
			Encoding: converter.EncodingJSON,
		},
	}
}

func (f *FlusherElasticSearch) Init(context pipeline.Context) error {
	f.context = context
	// Validate config of flusher
	if err := f.Validate(); err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init elasticsearch flusher fail, error", err)
		return err
	}
	// Set default value while not set
	if f.Convert.Encoding == "" {
		f.converter.Encoding = converter.EncodingJSON
	}
	if f.Convert.Protocol == "" {
		f.Convert.Protocol = converter.ProtocolCustomSingle
	}
	// Init converter
	convert, err := f.getConverter()
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init clickhouse flusher converter fail, error", err)
		return err
	}
	f.converter = convert

	cfg := f.initClientCfg()
	f.esClient, err = elasticsearch.NewClient(cfg)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "create elasticsearch client error", err)
		return err
	}
	return nil
}

func (f *FlusherElasticSearch) Description() string {
	return "ElasticSearch flusher for logtail"
}

func (f *FlusherElasticSearch) initClientCfg() elasticsearch.Config {
	cfg := elasticsearch.Config{
		Addresses: f.Addresses,
	}
	if f.Authentication.PlainText.Username != "" {
		cfg.Username = f.Authentication.PlainText.Username
	}
	if f.Authentication.PlainText.Password != "" {
		cfg.Password = f.Authentication.PlainText.Password
	}
	if len(f.Authentication.cert) != 0 {
		cfg.CACert = f.Authentication.cert
	}
	return cfg
}

func (f *FlusherElasticSearch) Validate() error {
	if f.Addresses == nil || len(f.Addresses) == 0 {
		var err = fmt.Errorf("elasticsearch addrs is nil")
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}
	return nil
}

func (f *FlusherElasticSearch) getConverter() (*converter.Converter, error) {
	logger.Debug(f.context.GetRuntimeContext(), "[ilogtail data convert config] Protocol", f.Convert.Protocol,
		"Encoding", f.Convert.Encoding, "TagFieldsRename", f.Convert.TagFieldsRename, "ProtocolFieldsRename", f.Convert.ProtocolFieldsRename)
	return converter.NewConverter(f.Convert.Protocol, f.Convert.Encoding, f.Convert.TagFieldsRename, f.Convert.ProtocolFieldsRename)
}

func (f *FlusherElasticSearch) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	return f.flusher(projectName, logstoreName, configName, logGroupList)
}

func (f *FlusherElasticSearch) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	return f.esClient != nil
}

func (f *FlusherElasticSearch) SetUrgent(flag bool) {}

func (f *FlusherElasticSearch) Stop() error {
	return nil
}

func (f *FlusherElasticSearch) NormalFlush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	for _, logGroup := range logGroupList {
		logger.Debug(f.context.GetRuntimeContext(), "[LogGroup] topic", logGroup.Topic, "logstore", logGroup.Category, "logcount", len(logGroup.Logs), "tags", logGroup.LogTags)
		serializedLogs, err := f.converter.ToByteStream(logGroup)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "flush elasticsearch convert log fail, error", err)
		}
		documentID := 0
		for _, log := range serializedLogs.([][]byte) {
			req := esapi.IndexRequest{
				Index:      f.Authentication.PlainText.Index,
				DocumentID: strconv.Itoa(documentID + 1),
				Body:       bytes.NewReader(log),
				Refresh:    "true",
			}
			res, err := req.Do(context.Background(), f.esClient)
			if err != nil {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "flush elasticsearch request fail, error", err)
			}
			defer res.Body.Close()
			if res.IsError() {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "[%s] Error indexing document ID=%d", res.Status(), documentID+1)
			}
			documentID++
		}
		logger.Debug(f.context.GetRuntimeContext(), "ClickHouse success send events: messageID")
	}

	return nil
}

func init() {
	pipeline.Flushers["flusher_elasticsearch"] = func() pipeline.Flusher {
		f := NewFlusherElasticSearch()
		f.flusher = f.NormalFlush
		return f
	}
}
