package defaultdecoder

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"

	"github.com/alibaba/ilogtail/helper/decoder/influxdb"
)

func TestInit_Should_Pass_The_Config_To_Real_Decoder(t *testing.T) {
	d := &ExtensionDefaultDecoder{}
	err := json.Unmarshal([]byte(`{"Format":"influxdb","FieldsExtend":true}`), d)
	assert.Nil(t, err)
	assert.Equal(t, map[string]interface{}{"FieldsExtend": true}, d.options)

	d.Init(nil)
	assert.NotNil(t, d.Decoder)
	assert.Equal(t, true, d.Decoder.(*influxdb.Decoder).FieldsExtend)
}
