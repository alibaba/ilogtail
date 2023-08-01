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

package statsd

import (
	"bytes"
	"context"
	"net/http"
	"sort"
	"strconv"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/protocol/decoder/common"

	dogstatsd "github.com/narqo/go-dogstatsd-parser"
	"github.com/prometheus/common/model"
)

const (
	metricNameKey = "__name__"
	labelsKey     = "__labels__"
	timeNanoKey   = "__time_nano__"
	valueKey      = "__value__"
)

type Decoder struct {
	Time time.Time
}

func parseLabels(metric *dogstatsd.Metric) (labelsValue string) {
	lns := make(model.LabelPairs, 0, len(metric.Tags))
	for k, v := range metric.Tags {
		lns = append(lns, &model.LabelPair{
			Name:  model.LabelName(k),
			Value: model.LabelValue(v),
		})
	}
	sort.Sort(lns)
	var builder strings.Builder
	labelCount := 0
	for _, label := range lns {
		if labelCount != 0 {
			builder.WriteByte('|')
		}
		k := string(label.Name)
		helper.ReplaceInvalidChars(&k)
		builder.WriteString(k)
		builder.WriteString("#$#")
		builder.WriteString(string(label.Value))
		labelCount++
	}
	return builder.String()
}

func (d *Decoder) Decode(data []byte, req *http.Request, tags map[string]string) (logs []*protocol.Log, err error) {
	now := time.Now()
	parts := bytes.Split(data, []byte("\n"))
	for _, part := range parts {
		if len(part) == 0 {
			continue
		}
		m, err := dogstatsd.Parse(string(part))
		if err != nil {
			logger.Debug(context.Background(), "parse statsd error", err)
			if time.Since(d.Time).Seconds() > 10 {
				logger.Error(context.Background(), "STATSD_PARSE_ALARM", "parse err", err)
				d.Time = time.Now()
			}
			continue
		}
		helper.ReplaceInvalidChars(&m.Name)
		log := &protocol.Log{
			Contents: []*protocol.Log_Content{
				{
					Key:   metricNameKey,
					Value: m.Name,
				},
				{
					Key:   labelsKey,
					Value: parseLabels(m),
				},
				{
					Key:   timeNanoKey,
					Value: strconv.FormatInt(now.UnixNano(), 10),
				},
				{
					Key:   valueKey,
					Value: strconv.FormatFloat(m.Value.(float64), 'g', -1, 64),
				},
			},
		}
		protocol.SetLogTime(log, uint32(now.Unix()), uint32(now.Nanosecond()))
		logs = append(logs, log)
	}
	return
}

func (d *Decoder) ParseRequest(res http.ResponseWriter, req *http.Request, maxBodySize int64) (data []byte, statusCode int, err error) {
	return common.CollectBody(res, req, maxBodySize)
}

func (d *Decoder) DecodeV2(data []byte, req *http.Request) (groups []*models.PipelineGroupEvents, err error) {
	//TODO: Implement DecodeV2
	return nil, nil
}
