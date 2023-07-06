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

package sls_metric

import (
	"fmt"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/plugins/test/mock"
	. "github.com/smartystreets/goconvey/convey"
	"testing"
)

func TestInitError(t *testing.T) {
	Convey("Init error.", t, func() {
		Convey("Regex error", func() {
			processor := &ProcessorSlsMetric{}
			err := processor.Init(mock.NewEmptyContext("p", "l", "c"))
			So(err, ShouldNotBeNil)
		})
	})
}

func TestProcessorSlsMetric_ProcessLogs(t *testing.T) {
	Convey("Given a set of logs", t, func() {
		logs := []*protocol.Log{
			{
				Time: 1234567890,
				Contents: []*protocol.Log_Content{
					{Key: "labelA", Value: "1"},
					{Key: "labelB", Value: "2"},
					{Key: "labelC", Value: "3"},
					{Key: "nameA", Value: "myname"},
					{Key: "valueA", Value: "1.0"},
					{Key: "nameB", Value: "myname2"},
					{Key: "valueB", Value: "2.0"},
					{Key: "__time__", Value: "1658806869597190887"},
				},
			},
			{
				Time: 1234567890,
				Contents: []*protocol.Log_Content{
					{Key: "labelB", Value: "22"},
					{Key: "labelC", Value: "33"},
					{Key: "nameB", Value: "myname2"},
					{Key: "valueB", Value: "2.0"},
					{Key: "__time__", Value: "1658806869597190887"},
					{Key: "valueA", Value: "1.0"},
					{Key: "labelA", Value: "11"},
					{Key: "nameA", Value: "myname1"},
				},
			},
			{
				Time: 1234567890,
				Contents: []*protocol.Log_Content{
					//{Key: "labelA", Value: "AAA"},
					{Key: "labelB", Value: "BBB"},
					{Key: "labelC", Value: "CCC"},
					{Key: "nameA", Value: "myname"},
					{Key: "valueA", Value: "1.0"},
					{Key: "nameB", Value: "myname2"},
					{Key: "valueB", Value: "2.0"},
					{Key: "__time__", Value: "1658806869597190887"},
				},
			},
		}

		Convey("When the logs are processed using ProcessorSlsMetric", func() {
			processor := &ProcessorSlsMetric{
				MetricTimeKey:   "__time__",
				MetricLabelKeys: []string{"labelA", "labelB", "labelC"},
				MetricValues: map[string]string{
					"nameA": "valueA",
					"nameB": "valueB",
				},
				CustomMetricLabels: map[string]string{
					"labelD": "CustomD",
				},
			}
			err := processor.Init(mock.NewEmptyContext("p", "l", "c"))
			So(err, ShouldBeNil)

			processedLogs := processor.ProcessLogs(logs)
			fmt.Println(processedLogs)
			Convey("Then the processed logs should have the correct format", func() {
				So(len(processedLogs), ShouldEqual, 4)

				So(processedLogs[0].Contents[0].Key, ShouldEqual, "__labels__")
				So(processedLogs[0].Contents[0].Value, ShouldEqual, "labelA#$#1|labelB#$#2|labelC#$#3|labelD#$#CustomD")
				So(processedLogs[0].Contents[1].Key, ShouldEqual, "__name__")
				So(processedLogs[0].Contents[1].Value, ShouldEqual, "myname")
				So(processedLogs[0].Contents[2].Key, ShouldEqual, "__value__")
				So(processedLogs[0].Contents[2].Value, ShouldEqual, "1.0")
				So(processedLogs[0].Contents[3].Key, ShouldEqual, "__time_nano__")
				So(processedLogs[0].Contents[3].Value, ShouldEqual, "1658806869597190887")

				So(processedLogs[1].Contents[0].Key, ShouldEqual, "__labels__")
				So(processedLogs[1].Contents[0].Value, ShouldEqual, "labelA#$#1|labelB#$#2|labelC#$#3|labelD#$#CustomD")
				So(processedLogs[1].Contents[1].Key, ShouldEqual, "__name__")
				So(processedLogs[1].Contents[1].Value, ShouldEqual, "myname2")
				So(processedLogs[1].Contents[2].Key, ShouldEqual, "__value__")
				So(processedLogs[1].Contents[2].Value, ShouldEqual, "2.0")
				So(processedLogs[1].Contents[3].Key, ShouldEqual, "__time_nano__")
				So(processedLogs[1].Contents[3].Value, ShouldEqual, "1658806869597190887")

				So(processedLogs[2].Contents[0].Key, ShouldEqual, "__labels__")
				So(processedLogs[2].Contents[0].Value, ShouldEqual, "labelA#$#11|labelB#$#22|labelC#$#33|labelD#$#CustomD")
				So(processedLogs[2].Contents[1].Key, ShouldEqual, "__name__")
				So(processedLogs[2].Contents[1].Value, ShouldEqual, "myname1")
				So(processedLogs[2].Contents[2].Key, ShouldEqual, "__value__")
				So(processedLogs[2].Contents[2].Value, ShouldEqual, "1.0")
				So(processedLogs[2].Contents[3].Key, ShouldEqual, "__time_nano__")
				So(processedLogs[2].Contents[3].Value, ShouldEqual, "1658806869597190887")

				So(processedLogs[3].Contents[0].Key, ShouldEqual, "__labels__")
				So(processedLogs[3].Contents[0].Value, ShouldEqual, "labelA#$#11|labelB#$#22|labelC#$#33|labelD#$#CustomD")
				So(processedLogs[3].Contents[1].Key, ShouldEqual, "__name__")
				So(processedLogs[3].Contents[1].Value, ShouldEqual, "myname2")
				So(processedLogs[3].Contents[2].Key, ShouldEqual, "__value__")
				So(processedLogs[3].Contents[2].Value, ShouldEqual, "2.0")
				So(processedLogs[3].Contents[3].Key, ShouldEqual, "__time_nano__")
				So(processedLogs[3].Contents[3].Value, ShouldEqual, "1658806869597190887")
			})
		})
	})

	Convey("Given a set of logs", t, func() {
		logs := []*protocol.Log{
			{
				Time: 1234567890,
				Contents: []*protocol.Log_Content{
					{Key: "labelA", Value: "AAA"},
					{Key: "nameA", Value: "myname"},
					{Key: "valueA", Value: "1.0"},
					{Key: "nameB", Value: "myname2"},
					{Key: "valueB", Value: "2.0"},
					{Key: "labelB", Value: "BBB"},
					{Key: "labelC", Value: "CCC"},
				},
			},
		}

		Convey("When the logs are processed using ProcessorSlsMetric", func() {
			processor := &ProcessorSlsMetric{
				MetricLabelKeys: []string{"labelA", "labelB", "labelC"},
				MetricValues: map[string]string{
					"nameA": "valueA",
					"nameB": "valueB",
				},
				CustomMetricLabels: map[string]string{
					"labelD": "CustomD",
				},
			}
			err := processor.Init(mock.NewEmptyContext("p", "l", "c"))
			So(err, ShouldBeNil)

			processedLogs := processor.ProcessLogs(logs)

			Convey("Then the processed logs should have the correct format", func() {
				So(len(processedLogs), ShouldEqual, 2)

				So(processedLogs[0].Contents[0].Key, ShouldEqual, "__labels__")
				So(processedLogs[0].Contents[0].Value, ShouldEqual, "labelA#$#AAA|labelB#$#BBB|labelC#$#CCC|labelD#$#CustomD")
				So(processedLogs[0].Contents[1].Key, ShouldEqual, "__name__")
				So(processedLogs[0].Contents[1].Value, ShouldEqual, "myname")
				So(processedLogs[0].Contents[2].Key, ShouldEqual, "__value__")
				So(processedLogs[0].Contents[2].Value, ShouldEqual, "1.0")
				So(processedLogs[0].Contents[3].Key, ShouldEqual, "__time_nano__")
				So(processedLogs[0].Contents[3].Value, ShouldEqual, "1234567890000000000")

				So(processedLogs[1].Contents[0].Key, ShouldEqual, "__labels__")
				So(processedLogs[1].Contents[0].Value, ShouldEqual, "labelA#$#AAA|labelB#$#BBB|labelC#$#CCC|labelD#$#CustomD")
				So(processedLogs[1].Contents[1].Key, ShouldEqual, "__name__")
				So(processedLogs[1].Contents[1].Value, ShouldEqual, "myname2")
				So(processedLogs[1].Contents[2].Key, ShouldEqual, "__value__")
				So(processedLogs[1].Contents[2].Value, ShouldEqual, "2.0")
				So(processedLogs[1].Contents[3].Key, ShouldEqual, "__time_nano__")
				So(processedLogs[1].Contents[3].Value, ShouldEqual, "1234567890000000000")
			})
		})
	})
}
