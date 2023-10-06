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

package oss

import (
	"errors"
	"strconv"
	"strings"

	"github.com/alibaba/ilogtail/pkg/fmtstr"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	converter "github.com/alibaba/ilogtail/pkg/protocol/converter"
	"github.com/aliyun/aliyun-oss-go-sdk/oss"
)

type FlusherOss struct {
	// oss endpoint
	Endpoint string
	Bucket   string
	// oss key
	KeyFormat          string
	Encoding           string
	ObjectAcl          string
	ObjectStorageClass string
	Tagging            string
	MaximumFileSize    int
	// Convert ilogtail data convert config
	Convert convertConfig
	// Authentication
	Authentication Authentication
	ContentKey     string

	indexKeys []string
	context   pipeline.Context
	converter *converter.Converter
	bucket    *oss.Bucket
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

func NewFlusherOss() *FlusherOss {
	return &FlusherOss{
		Endpoint:           "",
		KeyFormat:          "",
		Encoding:           "",
		ObjectAcl:          "",
		ObjectStorageClass: "",
		Tagging:            "",
		ContentKey:         "",
		MaximumFileSize:    0,
		Authentication: Authentication{
			PlainText: &PlainTextConfig{
				AccessKeyId:     "",
				AccessKeySecret: "",
			},
		},
		Convert: convertConfig{
			Protocol: converter.ProtocolCustomSingleFlatten,
			Encoding: converter.EncodingJSON,
		},
	}
}

func (f *FlusherOss) Init(context pipeline.Context) error {
	f.context = context
	// Validate config of flusher
	if err := f.Validate(); err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init oss flusher fail, error", err)
		return err
	}
	// Set default value while not set
	if f.Convert.Encoding == "" {
		f.converter.Encoding = converter.EncodingJSON
	}
	if f.Convert.Protocol == "" {
		f.Convert.Protocol = converter.ProtocolCustomSingleFlatten
	}
	// Init converter
	convert, err := f.getConverter()
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init oss flusher converter fail, error", err)
		return err
	}
	f.converter = convert

	if f.Endpoint == "" {
		return errors.New("endpoint can't be empty")
	}

	if f.Bucket == "" {
		return errors.New("bucket can't be empty")
	}

	if f.KeyFormat == "" {
		return errors.New("keyFormat can't be empty")
	}

	if f.ContentKey == "" {
		return errors.New("ContentKey can't be empty")
	}

	// Obtain index keys from dynamic index expression
	indexKeys, err := fmtstr.CompileKeys(f.KeyFormat)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init oss flusher index fail, error", err)
		return err
	}
	f.indexKeys = indexKeys

	ossClient, err := oss.New(f.Endpoint, f.Authentication.PlainText.AccessKeyId, f.Authentication.PlainText.AccessKeySecret)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "create oss client error", err)
		return err
	}
	f.bucket, err = ossClient.Bucket(f.Bucket)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init oss client bucket error", err)
		return err
	}
	return nil
}

func (f *FlusherOss) Description() string {
	return "oss flusher for logtail"
}

func (f *FlusherOss) Validate() error {
	if f.Endpoint == "" {
		return errors.New("endpoint is nil")
	}
	if len(f.KeyFormat) == 0 {
		return errors.New("key format is nil")
	}
	return nil
}

func (f *FlusherOss) getConverter() (*converter.Converter, error) {
	logger.Debug(f.context.GetRuntimeContext(), "[ilogtail data convert config] Protocol", f.Convert.Protocol,
		"Encoding", f.Convert.Encoding, "TagFieldsRename", f.Convert.TagFieldsRename, "ProtocolFieldsRename", f.Convert.ProtocolFieldsRename)
	return converter.NewConverter(f.Convert.Protocol, f.Convert.Encoding, f.Convert.TagFieldsRename, f.Convert.ProtocolFieldsRename)
}

func (f *FlusherOss) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	return f.bucket != nil
}

func (f *FlusherOss) SetUrgent(flag bool) {}

func (f *FlusherOss) Stop() error {
	return nil
}

func (f *FlusherOss) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	logContentMap := make(map[string]string)
	for _, logGroup := range logGroupList {
		logger.Debug(f.context.GetRuntimeContext(), "[LogGroup] topic", logGroup.Topic, "logstore", logGroup.Category, "logcount", len(logGroup.Logs), "tags", logGroup.LogTags)
		serializedLogs, values, err := f.converter.DoWithSelectedFields(logGroup, f.indexKeys)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "flush oss convert log fail, error", err)
		}
		for index, log := range serializedLogs.([]map[string]interface{}) {
			contentValue, ok := log[f.ContentKey]
			if !ok {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "flush oss get contentKey fail, error", err)
			}
			valueMap := values[index]
			objectKey, err := fmtstr.FormatPath(valueMap, f.KeyFormat)
			if err != nil {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "flush oss format path fail, error", err)
			} else {
				logContentMap[*objectKey] += (contentValue.(string) + "\n")
			}
		}
		logger.Debug(f.context.GetRuntimeContext(), "oss success send events: messageID")
	}

	for key, value := range logContentMap {
		f.flushKeyValue(key, value)
	}
	return nil
}

func (f *FlusherOss) flushKeyValue(key string, value string) error {
	var nextPos int64 = 0
	var err error
	exists, _ := f.bucket.IsObjectExist(key)
	if exists {
		props, err := f.bucket.GetObjectDetailedMeta(key)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "GetObjectDetailedMeta fail, err", err)
			return err
		}
		nextPos, err = strconv.ParseInt(props.Get(oss.HTTPHeaderOssNextAppendPosition), 10, 64)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "parse oss next append position fail, err", err)
			return err
		}
	}

	//if object exceed maximum filesize
	if nextPos > int64(f.MaximumFileSize) {
		destObject := key + "_1"
		_, err = f.bucket.CopyObject(key, destObject)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "copy oss object fail, object", key, "err", err)
			return err
		}
		err = f.bucket.DeleteObject(key)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "delete oss object fail, object", key, "err", err)
			return err
		}
		nextPos = 0
	}

	_, err = f.bucket.AppendObject(key, strings.NewReader(value), nextPos)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "append oss data fail, err", err)
		return err
	}
	return nil
}

func init() {
	pipeline.Flushers["flusher_oss"] = func() pipeline.Flusher {
		f := NewFlusherOss()
		return f
	}
}
