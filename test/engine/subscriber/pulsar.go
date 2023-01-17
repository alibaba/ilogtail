package subscriber

import (
	"context"
	"fmt"
	"time"

	"github.com/apache/pulsar-client-go/pulsar"
	"github.com/mitchellh/mapstructure"

	"github.com/alibaba/ilogtail/pkg/doc"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
)

const pulsarName = "pulsar"

type PulsarSubscriber struct {
	URL   string `mapstructure:"url" comment:"the url of pulsar"`
	Topic string `mapstructure:"topic" comment:"the url of pulsar"`

	client   pulsar.Client
	consumer pulsar.Consumer
	channel  chan *protocol.LogGroup
}

func (s *PulsarSubscriber) Name() string {
	return ""
}

func (s *PulsarSubscriber) Description() string {
	return "this a flusher pulsar  subscriber, which is the default mock backend for Ilogtail."
}

func (s *PulsarSubscriber) Start() error {
	clientOpts := pulsar.ClientOptions{
		URL: s.URL,
	}
	client, err := pulsar.NewClient(clientOpts)
	if err != nil {
		return fmt.Errorf("failed to create pulsar client: %s", err)
	}
	s.client = client
	logger.Info(context.Background(), "Pulsar Subscriber started")
	consumer, err := client.Subscribe(pulsar.ConsumerOptions{
		Topic:            s.Topic,
		SubscriptionName: "my-sub",
		Type:             pulsar.Shared,
	})
	if err != nil {
		return fmt.Errorf("failed to Subscribe from pulsar: %s", err)
	} else {
		s.consumer = consumer
		go func() {
			if err := s.consumeMessages(); err != nil {
				logger.Error(context.Background(), "consume logs loop occurs an error", err)
			}
		}()
	}
	return nil
}

func (s *PulsarSubscriber) Stop() {
	close(s.channel)
	return
}

func (s *PulsarSubscriber) SubscribeChan() <-chan *protocol.LogGroup {
	return s.channel
}

func (s *PulsarSubscriber) FlusherConfig() string {
	return ""
}

func (s *PulsarSubscriber) consumeMessages() error {
	for {
		msg, err := s.consumer.Receive(context.Background())
		if err != nil {
			return fmt.Errorf("failed to revevive message from pulsar: %s", err)
		}
		logGroup := s.pulsarMsgToLogGroup(msg)
		s.channel <- logGroup
	}
}

func (s *PulsarSubscriber) pulsarMsgToLogGroup(msg pulsar.Message) *protocol.LogGroup {
	fields := make(map[string]string)
	fields[string(msg.Key())] = string(msg.Payload())
	log, _ := util.CreateLog(time.Now(), nil, nil, fields)
	err := s.consumer.Ack(msg)
	if err != nil {
		logger.Error(context.Background(), "Pulsar Subscriber ack message failed", "err", err)
	}
	logGroup := &protocol.LogGroup{
		Logs: []*protocol.Log{},
	}
	logGroup.Logs = append(logGroup.Logs, log)
	return logGroup
}

func init() {
	RegisterCreator(pulsarName, func(spec map[string]interface{}) (Subscriber, error) {
		s := new(PulsarSubscriber)
		if err := mapstructure.Decode(spec, s); err != nil {
			return nil, err
		}
		if s.URL == "" {
			s.URL = "pulsar://pulsar:6650"
		}
		if s.Topic == "" {
			s.Topic = "test"
		}
		s.channel = make(chan *protocol.LogGroup, 10)
		return s, nil
	})
	doc.Register("subscriber", pulsarName, new(PulsarSubscriber))
}
