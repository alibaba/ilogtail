package jfr

import (
	"context"
	"errors"
	"fmt"
	"net/http"
	"strings"

	"github.com/alibaba/ilogtail/helper/decoder/common"
	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/alibaba/ilogtail/helper/profile/pprof"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
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
