package protocol

import (
	"fmt"
	"testing"

	"github.com/smartystreets/goconvey/convey"

	"github.com/alibaba/ilogtail/pkg/protocol"
)

func TestConvertToInfluxdbProtocolStream(t *testing.T) {
	convey.Convey("Given protocol.LogGroup", t, func() {
		cases := []struct {
			name       string
			logGroup   *protocol.LogGroup
			wantStream interface{}
			wantErr    bool
		}{
			{
				name: "contains metrics without labels or timestamp part",
				logGroup: &protocol.LogGroup{
					Logs: []*protocol.Log{
						{
							Contents: []*protocol.Log_Content{
								{Key: metricNameKey, Value: "metric:field"},
								{Key: metricLabelsKey, Value: ""},
								{Key: metricValueKey, Value: "1"},
							},
						},
						{
							Contents: []*protocol.Log_Content{
								{Key: metricNameKey, Value: "metric:field"},
								{Key: metricLabelsKey, Value: "aa#$#bb"},
								{Key: metricValueKey, Value: "1"},
							},
						},
						{
							Contents: []*protocol.Log_Content{
								{Key: metricNameKey, Value: "metric:field"},
								{Key: metricValueKey, Value: "1"},
								{Key: metricTimeNanoKey, Value: "1667615389000000000"},
							},
						},
					},
				},
				wantStream: []byte("metric field=1\nmetric,aa=bb field=1\nmetric field=1 1667615389000000000\n"),
			},
		}

		converter, err := NewConverter("influxdb", "none", nil, nil)
		convey.So(err, convey.ShouldBeNil)

		for _, test := range cases {
			convey.Convey(fmt.Sprintf("When %s, result should match", test.name), func() {

				stream, _, err := converter.ConvertToInfluxdbProtocolStream(test.logGroup, nil)
				if test.wantErr {
					convey.So(err, convey.ShouldNotBeNil)
				} else {
					convey.So(err, convey.ShouldBeNil)
					convey.So(stream, convey.ShouldResemble, test.wantStream)
				}
			})
		}
	})
}
