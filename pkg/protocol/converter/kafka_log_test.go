package protocol

import (
	"testing"

	"github.com/smartystreets/goconvey/convey"

	"github.com/alibaba/ilogtail/pkg/protocol"
)

func TestConvertToKafkaSingleLog(t *testing.T) {
	convey.Convey("Convert ilogtail log to kafka single log  ", t, func() {
		tagKeyRenameMap := map[string]string{
			"host.name": "hostname",
		}

		protocolKeyRenameMap := map[string]string{
			"time": "@timestamp",
		}

		c, _ := NewConverter("custom_single", "json", tagKeyRenameMap, protocolKeyRenameMap)

		convey.Convey("When performing conversion", func() {
			logs := []*protocol.Log{
				{
					Time: 1662434209,
					Contents: []*protocol.Log_Content{
						{Key: "app", Value: "kube-apiserver"},
						{Key: "method", Value: "PUT"},
					},
				}, {
					Time: 1662434209,
					Contents: []*protocol.Log_Content{
						{Key: "app", Value: "coredns"},
					},
				},
			}
			tags := []*protocol.LogTag{
				{Key: "__hostname__", Value: "alje834hgf"},
				{Key: "__pack_id__", Value: "AEDCFGHNJUIOPLMN-1E"},
				{Key: "__path__", Value: "/root/test/origin/example.log"},
				{Key: "__log_topic__", Value: "file"},
				{Key: "_namespace_", Value: "zookeeper"},
			}
			logGroup := &protocol.LogGroup{
				Logs:     logs,
				Category: "test",
				Topic:    "topic",
				Source:   "172.10.0.56",
				LogTags:  tags,
			}
			kafkaTopic := "test_%{content.app}"

			topicMap := map[string]string{
				"test_kube-apiserver": "test_kube-apiserver",
				"test_coredns":        "test_coredns",
			}
			for _, log := range logGroup.Logs {
				_, topic, err := c.ConvertToKafkaSingleLog(logGroup, log, kafkaTopic)
				convey.So(err, convey.ShouldBeNil)
				convey.So(topicMap[*topic], convey.ShouldEqual, *topic)
			}
		})
	})
}
