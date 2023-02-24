package extensions

import (
	"fmt"
	"net/http"

	"github.com/mitchellh/mapstructure"

	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

var decoderFactories = map[string]DecoderCreator{}

type DecoderCreator func() Decoder

// Decoder used to parse buffer to sls logs
type Decoder interface {
	// Decode reader to logs
	Decode(data []byte, req *http.Request, tags map[string]string) (logs []*protocol.Log, err error)
	// DecodeV2 reader to groupEvents
	DecodeV2(data []byte, req *http.Request) (groups []*models.PipelineGroupEvents, err error)
	// ParseRequest gets the request's body raw data and status code.
	ParseRequest(res http.ResponseWriter, req *http.Request, maxBodySize int64) (data []byte, statusCode int, err error)
}

func AddDecoderCreator(protocol string, creator DecoderCreator) {
	decoderFactories[protocol] = creator
}

func CreateDecoder(protocol string, config interface{}) (Decoder, error) {
	factory, ok := decoderFactories[protocol]
	if !ok {
		return nil, fmt.Errorf("decoder for protocol(%s) not found", protocol)
	}

	decoder := factory()
	err := mapstructure.Decode(config, decoder)
	if err != nil {
		return nil, err
	}
	return decoder, nil
}
