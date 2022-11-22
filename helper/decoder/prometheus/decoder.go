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

package prometheus

import (
	"bytes"
	"io"
	"net/http"
	"sort"
	"strconv"
	"strings"

	"github.com/alibaba/ilogtail/helper/decoder/common"
	"github.com/alibaba/ilogtail/pkg/protocol"

	"github.com/gogo/protobuf/proto"
	"github.com/prometheus/common/expfmt"
	"github.com/prometheus/common/model"
	"github.com/prometheus/prometheus/prompb"
)

const (
	metricNameKey = "__name__"
	labelsKey     = "__labels__"
	timeNanoKey   = "__time_nano__"
	valueKey      = "__value__"
)

const tagDB = "__tag__:db"

// Decoder impl
type Decoder struct {
}

func parseLabels(metric model.Metric) (metricName, labelsValue string) {
	labels := (model.LabelSet)(metric)

	lns := make(model.LabelPairs, 0, len(labels))
	for k, v := range labels {
		lns = append(lns, &model.LabelPair{
			Name:  k,
			Value: v,
		})
	}
	sort.Sort(lns)
	var builder strings.Builder
	labelCount := 0
	for _, label := range lns {
		if label.Name == model.MetricNameLabel {
			metricName = string(label.Value)
			continue
		}
		if labelCount != 0 {
			builder.WriteByte('|')
		}
		builder.WriteString(string(label.Name))
		builder.WriteString("#$#")
		builder.WriteString(string(label.Value))
		labelCount++
	}
	return metricName, builder.String()
}

// Decode impl
func (d *Decoder) Decode(data []byte, req *http.Request) (logs []*protocol.Log, err error) {
	if req.Header.Get("Content-Encoding") == "snappy" && strings.HasPrefix(req.Header.Get("Content-Type"), "application/x-protobuf") {
		return d.decodeInRemoteWriteFormat(data, req)
	}
	return d.decodeInExpFmt(data, req)
}

func (d *Decoder) decodeInExpFmt(data []byte, req *http.Request) (logs []*protocol.Log, err error) {
	decoder := expfmt.NewDecoder(bytes.NewReader(data), expfmt.FmtText)
	sampleDecoder := expfmt.SampleDecoder{
		Dec: decoder,
		Opts: &expfmt.DecodeOptions{
			Timestamp: model.Now(),
		},
	}

	for {
		s := &model.Vector{}
		if err = sampleDecoder.Decode(s); err != nil {
			if err == io.EOF {
				err = nil
				break
			}
			break
		}
		for _, sample := range *s {
			metricName, labelsValue := parseLabels(sample.Metric)
			log := &protocol.Log{
				Time: uint32(sample.Timestamp.Unix()),
				Contents: []*protocol.Log_Content{
					{
						Key:   metricNameKey,
						Value: metricName,
					},
					{
						Key:   labelsKey,
						Value: labelsValue,
					},
					{
						Key:   timeNanoKey,
						Value: strconv.FormatInt(sample.Timestamp.UnixNano(), 10),
					},
					{
						Key:   valueKey,
						Value: strconv.FormatFloat(float64(sample.Value), 'g', -1, 64),
					},
				},
			}
			logs = append(logs, log)
		}
	}

	return logs, err
}

func (d *Decoder) ParseRequest(res http.ResponseWriter, req *http.Request, maxBodySize int64) (data []byte, statusCode int, err error) {
	return common.CollectBody(res, req, maxBodySize)
}

func (d *Decoder) decodeInRemoteWriteFormat(data []byte, req *http.Request) (logs []*protocol.Log, err error) {
	var metrics prompb.WriteRequest
	err = proto.Unmarshal(data, &metrics)
	if err != nil {
		return nil, err
	}

	for _, m := range metrics.Timeseries {
		metricName, labelsValue := d.parsePbLabels(m.Labels)
		for _, sample := range m.Samples {
			log := &protocol.Log{
				Time: uint32(model.Time(sample.Timestamp).Unix()),
				Contents: []*protocol.Log_Content{
					{
						Key:   metricNameKey,
						Value: metricName,
					},
					{
						Key:   labelsKey,
						Value: labelsValue,
					},
					{
						Key:   timeNanoKey,
						Value: strconv.FormatInt(sample.Timestamp*1e6, 10),
					},
					{
						Key:   valueKey,
						Value: strconv.FormatFloat(float64(sample.Value), 'g', -1, 64),
					},
				},
			}
			logs = append(logs, log)
		}
	}

	db := req.FormValue("db")
	if db != "" {
		for _, log := range logs {
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: tagDB, Value: db})
		}
	}

	return logs, nil
}

func (d *Decoder) parsePbLabels(labels []prompb.Label) (metricName, labelsValue string) {
	var builder strings.Builder
	for _, label := range labels {
		if label.Name == model.MetricNameLabel {
			metricName = string(label.Value)
			continue
		}
		if builder.Len() > 0 {
			builder.WriteByte('|')
		}
		builder.WriteString(string(label.Name))
		builder.WriteString("#$#")
		builder.WriteString(string(label.Value))
	}
	return metricName, builder.String()
}
