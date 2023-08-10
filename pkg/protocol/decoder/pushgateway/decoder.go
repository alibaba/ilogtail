package pushgateway

import (
	parserCommon "github.com/VictoriaMetrics/VictoriaMetrics/lib/protoparser/common"
	"github.com/VictoriaMetrics/VictoriaMetrics/lib/protoparser/prometheus"
	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/protocol/decoder/common"
	"github.com/alibaba/ilogtail/pkg/util"
	"net/http"
	"time"
)

type Decoder struct {
}

func (d *Decoder) Decode(data []byte, req *http.Request, tags map[string]string) (logs []*protocol.Log, err error) {
	extraLabels, err := parserCommon.GetExtraLabels(req)
	if err != nil {
		return nil, err
	}
	defaultTimestamp, err := parserCommon.GetTimestamp(req)
	if err != nil {
		return nil, err
	}
	if defaultTimestamp <= 0 {
		defaultTimestamp = time.Now().UnixNano() / 1e6
	}
	var rows prometheus.Rows
	rows.Unmarshal(util.ZeroCopyBytesToString(data))
	if len(rows.Rows) == 0 {
		return nil, nil
	}
	var el helper.MetricLabels
	for _, tag := range extraLabels {
		el.Append(tag.Name, tag.Value)
	}
	for k, v := range tags {
		el.Append(k, v)
	}
	var labels *helper.MetricLabels
	for i := range rows.Rows {
		r := &rows.Rows[i]
		if r.Timestamp == 0 {
			r.Timestamp = defaultTimestamp
		}
		var log *protocol.Log
		if len(r.Tags) == 0 {
			log = helper.NewMetricLog(r.Metric, r.Timestamp, r.Value, &el)
		} else {
			labels = el.CloneInto(labels)
			for j := range r.Tags {
				labels.Append(r.Tags[j].Key, r.Tags[j].Value)
			}
			log = helper.NewMetricLog(r.Metric, r.Timestamp, r.Value, labels)
		}
		logs = append(logs, log)
	}
	return logs, nil
}

func (d *Decoder) DecodeV2(data []byte, req *http.Request) (groups []*models.PipelineGroupEvents, err error) {
	return nil, nil
}

func (d *Decoder) ParseRequest(res http.ResponseWriter, req *http.Request, maxBodySize int64) (data []byte, statusCode int, err error) {
	return common.CollectBody(res, req, maxBodySize)
}
