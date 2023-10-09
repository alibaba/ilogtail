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
	"net/http"
	"strconv"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/pkg/fmtstr"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	converter "github.com/alibaba/ilogtail/pkg/protocol/converter"
	"github.com/aliyun/aliyun-oss-go-sdk/oss"
)

type FlusherOss struct {
	Endpoint           string
	Bucket             string
	KeyFormat          string
	ContentKey         string
	Encoding           string
	ObjectAcl          string
	ObjectStorageClass string
	Tagging            string
	MaximumFileSize    int64
	// Convert ilogtail data convert config
	Convert convertConfig
	// Authentication
	Authentication Authentication

	pathKeys  []string
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
		Bucket:             "",
		KeyFormat:          "",
		ContentKey:         "",
		Encoding:           "",
		ObjectAcl:          "",
		ObjectStorageClass: "",
		Tagging:            "",
		MaximumFileSize:    4 * 1024 * 1024 * 1024,
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
	// Obtain index keys from dynamic index expression
	pathKeys, err := fmtstr.CompileKeys(f.KeyFormat)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init oss flusher index fail, error", err)
		return err
	}
	f.pathKeys = pathKeys

	var ossClient *oss.Client
	// read ak and sk from env params
	provider, err := oss.NewEnvironmentVariableCredentialsProvider()
	if err != nil {
		ossClient, err = oss.New(f.Endpoint, f.Authentication.PlainText.AccessKeyId, f.Authentication.PlainText.AccessKeySecret)
	} else {
		ossClient, err = oss.New(f.Endpoint, f.Authentication.PlainText.AccessKeyId, f.Authentication.PlainText.AccessKeySecret, oss.SetCredentialsProvider(&provider))
	}
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
	if len(f.Endpoint) == 0 {
		return errors.New("endpoint can't be empty")
	}
	if len(f.Bucket) == 0 {
		return errors.New("bucket can't be empty")
	}
	if len(f.KeyFormat) == 0 {
		return errors.New("keyFormat can't be empty")
	}
	if len(f.ContentKey) == 0 {
		return errors.New("contentKey can't be empty")
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
		serializedLogs, values, err := f.converter.DoWithSelectedFields(logGroup, f.pathKeys)
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
		_ = f.flushKeyValue(key, value)
	}
	return nil
}

func (f *FlusherOss) flushKeyValue(key string, value string) error {
	var nextPos int64
	var err error
	exists, _ := f.bucket.IsObjectExist(key)
	if exists {
		var props http.Header
		props, err = f.bucket.GetObjectDetailedMeta(key)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "get object detail meta fail, err", err)
			return err
		}
		nextPos, err = strconv.ParseInt(props.Get(oss.HTTPHeaderOssNextAppendPosition), 10, 64)
		if err != nil {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "parse oss next append position fail, err", err)
			return err
		}

		// if object exceed maximum filesize, backup old file and make new
		if nextPos > f.MaximumFileSize {
			destObject := key + "." + strconv.FormatInt(time.Now().Local().Unix(), 10)
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
		} else {
			// append content at the end of position
			_, err = f.bucket.AppendObject(key, strings.NewReader(value), nextPos)
			if err != nil {
				logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "append oss data fail, err", err)
			}
			return err
		}
	}
	// initial append
	_, err = f.bucket.AppendObject(key, strings.NewReader(value), nextPos, f.initOption()...)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "append oss data fail, err", err)
		return err
	}
	return nil
}

func (f *FlusherOss) initOption() []oss.Option {
	option := []oss.Option{}
	if len(f.Encoding) > 0 {
		option = append(option, oss.ContentEncoding(contentEncodingMethod(f.Encoding)))
	}
	if len(f.ObjectAcl) > 0 {
		option = append(option, oss.ObjectACL(objectAclMethod(f.ObjectAcl)))
	}
	if len(f.ObjectStorageClass) > 0 {
		option = append(option, oss.ObjectStorageClass(objectStorageClassMethod(f.ObjectStorageClass)))
	}
	if len(f.Tagging) > 0 {
		option = append(option, oss.SetTagging(taggingMethod(f.Tagging)))
	}
	return option
}

func contentEncodingMethod(contentEncoding string) string {
	switch contentEncoding {
	case "gzip", "compress", "deflate", "br":
		return contentEncoding
	default:
		return "identity"
	}
}

func objectAclMethod(objectAcl string) oss.ACLType {
	switch strings.ToLower(objectAcl) {
	case "private":
		return oss.ACLPrivate
	case "public-read":
		return oss.ACLPublicRead
	case "public-read-write":
		return oss.ACLPublicReadWrite
	default:
		return oss.ACLDefault
	}
}

func objectStorageClassMethod(objectStorageClass string) oss.StorageClassType {
	switch strings.ToLower(objectStorageClass) {
	case "ia":
		return oss.StorageIA
	case "archive":
		return oss.StorageArchive
	case "coldarchive":
		return oss.StorageColdArchive
	case "deepcoldarchive":
		return oss.StorageDeepColdArchive
	default:
		return oss.StorageStandard
	}
}

func taggingMethod(tagging string) oss.Tagging {
	tags := []oss.Tag{}
	for _, keyValuePair := range strings.Split(tagging, "&") {
		keyValue := strings.Split(keyValuePair, "=")
		if len(keyValue) == 2 {
			tag := oss.Tag{
				Key:   keyValue[0],
				Value: keyValue[1],
			}
			tags = append(tags, tag)
		}
	}
	return oss.Tagging{
		Tags: tags,
	}
}

func init() {
	pipeline.Flushers["flusher_oss"] = func() pipeline.Flusher {
		f := NewFlusherOss()
		return f
	}
}
