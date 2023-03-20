package extensions

import (
	"net/http"

	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

// Decoder used to parse buffer to sls logs
type Decoder interface {
	// Decode reader to logs
	Decode(data []byte, req *http.Request, tags map[string]string) (logs []*protocol.Log, err error)
	// DecodeV2 reader to groupEvents
	DecodeV2(data []byte, req *http.Request) (groups []*models.PipelineGroupEvents, err error)
	// ParseRequest gets the request's body raw data and status code.
	ParseRequest(res http.ResponseWriter, req *http.Request, maxBodySize int64) (data []byte, statusCode int, err error)
}
