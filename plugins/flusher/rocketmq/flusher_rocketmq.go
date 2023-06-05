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

package rocketmq

import (
	"context"
	"errors"
	"fmt"
	"time"

	"github.com/apache/rocketmq-client-go/v2"
	"github.com/apache/rocketmq-client-go/v2/primitive"
	"github.com/apache/rocketmq-client-go/v2/producer"

	"github.com/alibaba/ilogtail/pkg/fmtstr"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	converter "github.com/alibaba/ilogtail/pkg/protocol/converter"
)

const (
	PartitionerTypeRandom     = "random"
	PartitionerTypeRoundRobin = "roundrobin"
	PartitionerTypeRoundHash  = "hash"
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

type FlusherRocketmq struct {
	NameServers []string `json:"NameServers" comment:"name server address"`
	Topic       string   `json:"Topic" comment:"rocketmq topic"`
	Sync        bool     `json:"Sync" comment:"send 2 rocketmq via sync or not"`

	MaxRetries int `json:"MaxRetries" comment:"send 2 rocketmq via sync or not"`

	// The default is 3s.
	BrokerTimeout int `json:"BrokerTimeout" comment:"send 2 rocketmq client timeout"`

	// ilogtail data convert config
	Convert convertConfig

	PartitionerType string `json:"PartitionerType" comment:"option of random roundrobin hash"`

	CompressionLevel int `json:"CompressionLevel" comment:"compress level range 0~9, 0 stands for best speed, 9 stands for best compression ratio"`

	CompressMsgBodyOverHowmuch int `json:"CompressMsgBodyOverHowmuch" comment:"specifies the threshold size of a message body for compression. If the message body size is larger than the specified value, it will be compressed before sending it to the broker. By default, the value is set to 4KB"`

	ProducerGroupName string `json:"ProducerGroupName" comment:"producer group name default LOGSTASH_PRODUCER"`

	context   pipeline.Context
	converter *converter.Converter
	flusher   FlusherFunc
	producer  rocketmq.Producer
	topicKeys []string
}

func (r *FlusherRocketmq) Description() string {
	return "rocketmq flusher for logtail"
}

func makePartitioner(r *FlusherRocketmq) (partitioner producer.QueueSelector, err error) {
	switch r.PartitionerType {
	case PartitionerTypeRoundRobin:
		partitioner = producer.NewRoundRobinQueueSelector()
	case PartitionerTypeRoundHash:
		partitioner = producer.NewHashQueueSelector()
	case PartitionerTypeRandom:
		partitioner = producer.NewRandomQueueSelector()
	default:
		return nil, fmt.Errorf("invalid PartitionerType,configured value %v", r.PartitionerType)
	}
	return partitioner, nil
}

func NewFlusherRocketmq() *FlusherRocketmq {
	return &FlusherRocketmq{
		NameServers:   nil,
		Sync:          true,
		MaxRetries:    3,
		BrokerTimeout: 5,
		Convert: convertConfig{
			Protocol: converter.ProtocolCustomSingle,
			Encoding: converter.EncodingJSON,
		},
		CompressionLevel:           5,
		PartitionerType:            PartitionerTypeRandom,
		CompressMsgBodyOverHowmuch: 4096,
		ProducerGroupName:          "LOGSTASH_PRODUCER",
	}
}

func (r *FlusherRocketmq) Init(context pipeline.Context) error {
	r.context = context
	if r.NameServers == nil || len(r.NameServers) == 0 {
		err := errors.New("name server is nil")
		return err
	}
	if r.Convert.Encoding == "" {
		r.converter.Encoding = converter.EncodingJSON
	}

	if r.Convert.Protocol == "" {
		r.converter.Protocol = converter.ProtocolCustomSingle
	}

	convert, err := r.getConverter()
	if err != nil {
		return err
	}
	r.converter = convert

	// Obtain topic keys from dynamic topic expression
	topicKeys, err := fmtstr.CompileKeys(r.Topic)
	if err != nil {
		return err
	}
	r.topicKeys = topicKeys

	partitioner, err := makePartitioner(r)
	if err != nil {
		return err
	}
	p, err := rocketmq.NewProducer(
		producer.WithNsResolver(primitive.NewPassthroughResolver(r.NameServers)),
		producer.WithRetry(r.MaxRetries),
		producer.WithSendMsgTimeout(time.Duration(r.BrokerTimeout)*time.Second),
		producer.WithQueueSelector(partitioner),
		producer.WithCompressMsgBodyOverHowmuch(r.CompressMsgBodyOverHowmuch),
		producer.WithGroupName(r.ProducerGroupName),
	)

	if err != nil {
		return err
	}

	err = p.Start()
	if err != nil {
		return err
	}
	r.producer = p
	return nil
}

func (r *FlusherRocketmq) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	var msgs []*primitive.Message
	for _, logGroup := range logGroupList {
		logger.Debug(r.context.GetRuntimeContext(), "[LogGroup] topic", logGroup.Topic, "logstore", logGroup.Category, "logcount", len(logGroup.Logs), "tags", logGroup.LogTags)
		logs, values, err := r.converter.ToByteStreamWithSelectedFields(logGroup, r.topicKeys)
		if err != nil {
			logger.Error(r.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "flush kafka convert log fail, error", err)
		}
		for index, log := range logs.([][]byte) {
			valueMap := values[index]
			topic := r.Topic
			if len(r.topicKeys) > 0 {
				formatedTopic, err := fmtstr.FormatTopic(valueMap, r.Topic)
				if err != nil {
					logger.Error(r.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "flush rocketmq format topic fail, error", err)
				} else {
					topic = *formatedTopic
				}
			}

			m := primitive.NewMessage(
				topic,
				log)
			msgs = append(msgs, m)
		}
	}
	if len(msgs) > 0 {
		if r.Sync {
			res, err := r.producer.SendSync(r.context.GetRuntimeContext(), msgs...)
			if err != nil {
				logger.Error(r.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "flush rocketmq error, error", err)
				return err
			}
			logger.Debug(r.context.GetRuntimeContext(), "success flush 2 rocketmq with [LogGroup] projectName", projectName, "logstoreName", logstoreName, "logcount", len(msgs), "res", res.String())
		} else {
			err := r.producer.SendAsync(r.context.GetRuntimeContext(),
				func(ctx context.Context, result *primitive.SendResult, e error) {
					if e != nil {
						logger.Error(r.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "flush rocketmq error, error", e)
					} else {
						logger.Debug(r.context.GetRuntimeContext(), "success flush 2 rocketmq with [LogGroup] projectName", projectName, "logstoreName", logstoreName, "logcount", len(msgs), "res", result.String())
					}
				},
				msgs...)
			if err != nil {
				logger.Error(r.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "flush rocketmq error, error", err)
				return err
			}
		}
	}
	return nil
}

func (r *FlusherRocketmq) Validate() error {
	if len(r.NameServers) == 0 {
		return errors.New("no nameserver configured")
	}
	// check topic
	if r.Topic == "" {
		return errors.New("topic can't be empty")
	}

	return nil
}

type FlusherFunc func(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error

func (r *FlusherRocketmq) getConverter() (*converter.Converter, error) {
	logger.Debug(r.context.GetRuntimeContext(), "[ilogtail data convert config] Protocol", r.Convert.Protocol,
		"Encoding", r.Convert.Encoding, "TagFieldsRename", r.Convert.TagFieldsRename, "ProtocolFieldsRename", r.Convert.ProtocolFieldsRename)
	return converter.NewConverter(r.Convert.Protocol, r.Convert.Encoding, r.Convert.TagFieldsRename, r.Convert.ProtocolFieldsRename)
}

func init() {
	pipeline.Flushers["flusher_rocketmq"] = func() pipeline.Flusher {
		f := NewFlusherRocketmq()
		f.flusher = f.Flush
		return f
	}
}

func (*FlusherRocketmq) SetUrgent(flag bool) {
}

// IsReady is ready to flush
func (r *FlusherRocketmq) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	return r.producer != nil
}

// Stop ...
func (r *FlusherRocketmq) Stop() error {
	err := r.producer.Shutdown()
	return err
}
