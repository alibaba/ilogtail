package extensions

import (
	"net/http"
	"testing"

	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/stretchr/testify/assert"
)

func TestCreateDecoder(t *testing.T) {
	AddDecoderCreator("test_protocol", func() Decoder {
		return &mockDecoder{}
	})
	config := map[string]interface{}{
		"Name": "hello",
	}

	decoder, err := CreateDecoder("test_protocol", config)
	assert.Nil(t, err)
	assert.NotNil(t, decoder)
	md, ok := decoder.(*mockDecoder)
	assert.True(t, ok)
	assert.NotNil(t, md)
	assert.Equal(t, "hello", md.Name)
}

type mockDecoder struct {
	Name string
}

func (m *mockDecoder) Decode(data []byte, req *http.Request, tags map[string]string) (logs []*protocol.Log, err error) {
	//TODO implement me
	panic("implement me")
}

func (m *mockDecoder) DecodeV2(data []byte, req *http.Request) (groups []*models.PipelineGroupEvents, err error) {
	//TODO implement me
	panic("implement me")
}

func (m *mockDecoder) ParseRequest(res http.ResponseWriter, req *http.Request, maxBodySize int64) (data []byte, statusCode int, err error) {
	//TODO implement me
	panic("implement me")
}
