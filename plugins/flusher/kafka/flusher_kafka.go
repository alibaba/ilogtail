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

package kafka

import (
	"encoding/json"
	"errors"
	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"strings"
	"sync"
	"time"

	"github.com/Shopify/sarama"
)

var (
	hashKey sarama.StringEncoder
)

type FlusherKafka struct {
	context         ilogtail.Context
	Brokers         []string
	SASLUsername    string
	SASLPassword    string
	Topic           string
	PartitionerType string
	ClientID        string
	isTerminal      chan bool
	producer        sarama.AsyncProducer
	HashKey         []string
	once            sync.Once //for sidecar mode load config once
	RunMode         string    //sidecar or daemonset or other
}

func (k *FlusherKafka) Init(context ilogtail.Context) error {
	k.context = context
	if k.Brokers == nil || len(k.Brokers) == 0 {
		var err = errors.New("brokers ip is nil")
		logger.Error(k.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init kafka flusher fail, error", err)
		return err
	}
	config := sarama.NewConfig()
	if len(k.SASLUsername) == 0 {
		logger.Warning(k.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "SASL information is not set, access Kafka server without authentication")
	} else {
		config.Net.SASL.Enable = true
		config.Net.SASL.User = k.SASLUsername
		config.Net.SASL.Password = k.SASLPassword
	}
	config.ClientID = k.ClientID
	config.Producer.Return.Successes = true
	// config.Producer.RequiredAcks = sarama.WaitForAll
	partitioner := sarama.NewRandomPartitioner
	switch k.PartitionerType {
	case "roundrobin":
		partitioner = sarama.NewRoundRobinPartitioner
	case "hash":
		partitioner = sarama.NewHashPartitioner
	case "random":
		partitioner = sarama.NewRandomPartitioner
	default:
		logger.Error(k.context.GetRuntimeContext(), "INVALID_KAFKA_PARTITIONER", "invalid PartitionerType, use RandomPartitioner instead, type", k.PartitionerType)
	}
	config.Producer.Partitioner = partitioner
	config.Producer.Timeout = 5 * time.Second
	producer, err := sarama.NewAsyncProducer(k.Brokers, config)
	if err != nil {
		logger.Error(k.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init kafka flusher fail, error", err)
		return err
	}
	SIGTERM := make(chan bool)
	go func(p sarama.AsyncProducer, SIGTERM chan bool) {
		errors := p.Errors()
		success := p.Successes()
		for {
			select {
			case err := <-errors:
				if err != nil {
					logger.Error(k.context.GetRuntimeContext(), "FLUSHER_FLUSH_ALARM", "flush kafka write data fail, error", err)
				}
			case <-success:
				// Do Nothing
			case <-SIGTERM:
				return
			}
		}
	}(producer, SIGTERM)

	k.producer = producer
	k.isTerminal = SIGTERM
	return nil
}

func (k *FlusherKafka) Description() string {
	return "Kafka flusher for logtail"
}

func (k *FlusherKafka) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	for _, logGroup := range logGroupList {
		logger.Debug(k.context.GetRuntimeContext(), "[LogGroup] topic", logGroup.Topic, "logstore", logGroup.Category, "logcount", len(logGroup.Logs), "tags", logGroup.LogTags)

		for _, log := range logGroup.Logs {
			buf, _ := json.Marshal(log)
			logger.Debug(k.context.GetRuntimeContext(), string(buf))
			m := &sarama.ProducerMessage{
				Topic: k.Topic,
				Value: sarama.ByteEncoder(buf),
			}
			//set key when partition type is hash
			if k.PartitionerType == "hash" {
				keyFunc := func() {
					var hashData []string
					for _, content := range log.GetContents() {
						if contains(k.HashKey, content.Key) {
							hashData = append(hashData, content.Value)
						}
					}
					if len(hashData) == 0 {
						hashData = append(hashData, logstoreName)
					}
					logger.Debug(k.context.GetRuntimeContext(), "partition key", hashData)
					hashKey = sarama.StringEncoder(strings.Join(hashData, ""))
				}
				if k.RunMode == "sidecar" {
					k.once.Do(keyFunc)
				} else {
					keyFunc()
				}
				m.Key = hashKey
			}

			k.producer.Input() <- m
		}
	}

	return nil
}

func (*FlusherKafka) SetUrgent(flag bool) {
}

// IsReady is ready to flush
func (k *FlusherKafka) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	return k.producer != nil
}

// Stop ...
func (k *FlusherKafka) Stop() error {
	err := k.producer.Close()
	close(k.isTerminal)
	return err
}

func init() {
	ilogtail.Flushers["flusher_kafka"] = func() ilogtail.Flusher {
		return &FlusherKafka{
			ClientID:        "LogtailPlugin",
			PartitionerType: "random",
		}
	}
}
func contains(s []string, e string) bool {
	for _, a := range s {
		if a == e {
			return true
		}
	}
	return false
}
