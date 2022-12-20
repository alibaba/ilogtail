package pyroscope

import (
	"context"
	"errors"
	"fmt"
	"net/http"
	"strconv"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/helper/decoder/common"
	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/alibaba/ilogtail/helper/profile/pprof"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"

	"github.com/pyroscope-io/pyroscope/pkg/storage/segment"
	"github.com/pyroscope-io/pyroscope/pkg/util/attime"
)

const AlarmType = "PYROSCOPE_ALARM"

type Decoder struct {
}

func (d *Decoder) Decode(data []byte, req *http.Request) (logs []*protocol.Log, err error) {
	in, ft, err := d.parseInputMeta(req)
	if err != nil {
		return nil, err
	}
	ct := req.Header.Get("Content-Type")
	switch {
	case ft == profile.FormatPprof:
		in.Profile = &pprof.RawProfile{
			RawData: data,
		}
	case strings.Contains(ct, "multipart/form-data"):
		in.Profile = &pprof.RawProfile{
			FormDataContentType: ct,
			RawData:             data,
		}
	default:
		st := fmt.Sprintf("unknown format type %s or content type %s", ft, ct)
		logger.Debug(context.Background(), st)
		return nil, errors.New(st)
	}
	return in.Profile.Parse(context.Background(), &in.Metadata)
}

func (d *Decoder) ParseRequest(res http.ResponseWriter, req *http.Request, maxBodySize int64) (data []byte, statusCode int, err error) {
	return common.CollectBody(res, req, maxBodySize)
}

func (d *Decoder) parseInputMeta(req *http.Request) (*profile.Input, profile.Format, error) {
	var input profile.Input
	q := req.URL.Query()

	n := q.Get("name")
	key, err := segment.ParseKey(n)
	if err != nil {
		logger.Error(context.Background(), AlarmType, "invalid name", n)
		return nil, "", fmt.Errorf("pyroscope protocol get name err: %w", err)
	}
	input.Metadata.Key = key

	if f := q.Get("from"); f != "" {
		input.Metadata.StartTime = attime.Parse(f)
	} else {
		input.Metadata.StartTime = time.Now()
	}

	if f := q.Get("until"); f != "" {
		input.Metadata.EndTime = attime.Parse(f)
	} else {
		input.Metadata.EndTime = time.Now()
	}

	input.Metadata.SampleRate = 100
	if sr := q.Get("sampleRate"); sr != "" {
		sampleRate, err := strconv.Atoi(sr)
		if err != nil {
			logger.Error(context.Background(), AlarmType, "invalid sampleRate", sr)
		} else {
			input.Metadata.SampleRate = uint32(sampleRate)
		}
	}

	if sn := q.Get("spyName"); sn != "" {
		input.Metadata.SpyName = sn
	} else {
		input.Metadata.SpyName = "unknown"
	}

	if u := q.Get("units"); u != "" {
		input.Metadata.Units = profile.Units(u)
	} else {
		input.Metadata.Units = profile.NanosecondsUnit
	}

	if at := q.Get("aggregationType"); at != "" {
		input.Metadata.AggregationType = profile.AggType(at)
	} else {
		input.Metadata.AggregationType = profile.SumAggType
	}

	format := q.Get("format")
	return &input, profile.Format(format), nil
}
