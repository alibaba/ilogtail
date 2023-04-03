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

	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/protocol/decoder/common"

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
	tagDB         = "__tag__:db"
	metaDBKey     = "db"
)

const (
	contentEncodingKey = "Content-Encoding"
	contentTypeKey     = "Content-Type"
	pbContentType      = "application/x-protobuf"
	snappyEncoding     = "snappy"
)

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
func (d *Decoder) Decode(data []byte, req *http.Request, tags map[string]string) (logs []*protocol.Log, err error) {
	if req.Header.Get(contentEncodingKey) == snappyEncoding &&
		strings.HasPrefix(req.Header.Get(contentTypeKey), pbContentType) {
		return d.decodeInRemoteWriteFormat(data, req)
	}
	return d.decodeInExpFmt(data, req)
}

func (d *Decoder) decodeInExpFmt(data []byte, _ *http.Request) (logs []*protocol.Log, err error) {
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

	db := req.FormValue(metaDBKey)
	contentLen := 4
	if len(db) > 0 {
		contentLen++
	}

	for _, m := range metrics.Timeseries {
		metricName, labelsValue := d.parsePbLabels(m.Labels)
		for _, sample := range m.Samples {
			contents := make([]*protocol.Log_Content, 0, contentLen)
			contents = append(contents, &protocol.Log_Content{
				Key:   metricNameKey,
				Value: metricName,
			}, &protocol.Log_Content{
				Key:   labelsKey,
				Value: labelsValue,
			}, &protocol.Log_Content{
				Key:   timeNanoKey,
				Value: strconv.FormatInt(sample.Timestamp*1e6, 10),
			}, &protocol.Log_Content{
				Key:   valueKey,
				Value: strconv.FormatFloat(sample.Value, 'g', -1, 64),
			})
			if len(db) > 0 {
				contents = append(contents, &protocol.Log_Content{
					Key:   tagDB,
					Value: db,
				})
			}

			log := &protocol.Log{
				Time:     uint32(model.Time(sample.Timestamp).Unix()),
				Contents: contents,
			}
			logs = append(logs, log)
		}
	}

	return logs, nil
}

func (d *Decoder) parsePbLabels(labels []prompb.Label) (metricName, labelsValue string) {
	var builder strings.Builder
	for _, label := range labels {
		if label.Name == model.MetricNameLabel {
			metricName = label.Value
			continue
		}
		if builder.Len() > 0 {
			builder.WriteByte('|')
		}
		builder.WriteString(label.Name)
		builder.WriteString("#$#")
		builder.WriteString(label.Value)
	}
	return metricName, builder.String()
}

func (d *Decoder) DecodeV2(data []byte, req *http.Request) (groups []*models.PipelineGroupEvents, err error) {
	if len(data) == 0 {
		return nil, nil
	}

	meta := models.NewMetadata()
	commonTags := models.NewTags()

	if db := req.FormValue(metaDBKey); len(db) > 0 {
		meta.Add(metaDBKey, db)
	}

	if req.Header.Get(contentEncodingKey) == snappyEncoding &&
		strings.HasPrefix(req.Header.Get(contentTypeKey), pbContentType) {
		var promRequest prompb.WriteRequest
		err = proto.Unmarshal(data, &promRequest)
		if err != nil {
			return nil, err
		}

		groupEvents, err := ConvertPromRequestToPipelineGroupEvents(&promRequest, meta, commonTags)
		if err != nil {
			return nil, err
		}
		return []*models.PipelineGroupEvents{groupEvents}, nil
	}

	return ConvertExpFmtDataToPipelineGroupEvents(data, meta, commonTags)
}

func ConvertExpFmtDataToPipelineGroupEvents(data []byte, metaInfo models.Metadata, commonTags models.Tags) (pg []*models.PipelineGroupEvents, err error) {
	decoder := expfmt.NewDecoder(bytes.NewReader(data), expfmt.FmtText)
	sampleDecoder := expfmt.SampleDecoder{
		Dec: decoder,
		Opts: &expfmt.DecodeOptions{
			Timestamp: model.Now(),
		},
	}

	for {
		s := model.Vector{}
		if err = sampleDecoder.Decode(&s); err != nil {
			if err == io.EOF {
				err = nil
				break
			}
			break
		}
		groupEvents, err := ConvertVectorToPipelineGroupEvents(&s, metaInfo, commonTags)
		if err == nil {
			pg = append(pg, groupEvents)
		}
	}

	return
}

func ConvertVectorToPipelineGroupEvents(s *model.Vector, metaInfo models.Metadata, commonTags models.Tags) (*models.PipelineGroupEvents, error) {
	groupEvent := &models.PipelineGroupEvents{
		Group: models.NewGroup(metaInfo, commonTags),
	}

	for _, sample := range *s {
		metricName, tags := parseModelMetric(sample.Metric)
		metric := models.NewSingleValueMetric(
			metricName,
			models.MetricTypeGauge,
			tags,
			sample.Timestamp.UnixNano(),
			sample.Value,
		)
		groupEvent.Events = append(groupEvent.Events, metric)
	}

	return groupEvent, nil
}

func parseModelMetric(metric model.Metric) (string, models.Tags) {
	var metricName string
	tags := models.NewTags()
	labels := (model.LabelSet)(metric)

	for k, v := range labels {
		if k == model.MetricNameLabel {
			metricName = string(v)
			continue
		}
		tags.Add(string(k), string(v))
	}

	return metricName, tags
}

func ConvertPromRequestToPipelineGroupEvents(promRequest *prompb.WriteRequest, metaInfo models.Metadata, commonTags models.Tags) (*models.PipelineGroupEvents, error) {
	groupEvent := &models.PipelineGroupEvents{
		Group: models.NewGroup(metaInfo, commonTags),
	}

	for _, ts := range promRequest.Timeseries {
		var metricName string
		tags := models.NewTags()
		for _, label := range ts.Labels {
			if label.Name == metricNameKey {
				metricName = label.Value
				continue
			}
			tags.Add(label.Name, label.Value)
		}

		for _, dataPoint := range ts.Samples {
			metric := models.NewSingleValueMetric(
				metricName,
				models.MetricTypeGauge,
				tags,
				model.Time(dataPoint.GetTimestamp()).Time().UnixNano(),
				dataPoint.GetValue(),
			)
			groupEvent.Events = append(groupEvent.Events, metric)
		}
	}

	return groupEvent, nil
}
