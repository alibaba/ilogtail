package cloudmeta

import (
	_ "github.com/alibaba/ilogtail/pkg/logger/test"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/plugins/processor/cloudmeta/platformmeta"
	"github.com/alibaba/ilogtail/plugins/test"
	"github.com/alibaba/ilogtail/plugins/test/mock"
	"github.com/stretchr/testify/require"
	"testing"
)

func Test_cloudMeta_ProcessLogs(t *testing.T) {
	c := new(ProcessorCloudMeta)
	c.Platform = platformmeta.Mock
	c.Mode = contentMode
	c.AddMetas = map[string]string{
		"instance_id":   "__instance_id__",
		"instance_name": "__instance_name__",
		"zone":          "__zone__",
		"region":        "__region__",
		"instance_type": "__instance_type__",
		"vswitch_id":    "__vswitch_id__",
		"vpc_id":        "__vpc_id__",
		"image_id":      "__image_id__",
		"max_ingress":   "__max_ingress__",
		"max_egress":    "__max_egress__",
		"instance_tags": "__instance_tags__",
	}
	require.NoError(t, c.Init(mock.NewEmptyContext("a", "b", "c")))

	log := &protocol.Log{
		Time: 1,
	}
	logs := c.ProcessLogs([]*protocol.Log{log})
	require.Equal(t, len(logs), 1)
	require.Equal(t, len(logs[0].Contents), 11)
	require.Equal(t, test.ReadLogVal(logs[0], "__instance_id__"), "id_xxx")
	require.Equal(t, test.ReadLogVal(logs[0], "__instance_name__"), "name_xxx")
	require.Equal(t, test.ReadLogVal(logs[0], "__zone__"), "zone_xxx")
	require.Equal(t, test.ReadLogVal(logs[0], "__region__"), "region_xxx")
	require.Equal(t, test.ReadLogVal(logs[0], "__instance_type__"), "type_xxx")
	require.Equal(t, test.ReadLogVal(logs[0], "__vswitch_id__"), "vswitch_xxx")
	require.Equal(t, test.ReadLogVal(logs[0], "__vpc_id__"), "vpc_xxx")
	require.Equal(t, test.ReadLogVal(logs[0], "__image_id__"), "image_xxx")
	require.Equal(t, test.ReadLogVal(logs[0], "__max_ingress__"), "100")
	require.Equal(t, test.ReadLogVal(logs[0], "__max_egress__"), "1000")
	require.Equal(t, test.ReadLogVal(logs[0], "__instance_tags___tag_key"), "tag_val")
}

func Test_cloudMeta_ProcessJsonLogs(t *testing.T) {
	metas := map[string]string{
		"instance_id":   "__instance_id__",
		"instance_name": "__instance_name__",
		"instance_tags": "__instance_tags__",
	}
	type fields struct {
		JsonContentKey  string
		JsonContentPath string
	}
	tests := []struct {
		name           string
		fields         fields
		initError      bool
		content        string
		key            string
		logsLen        int
		logsContentLen int
		validator      func(log *protocol.Log, t *testing.T)
	}{
		{
			name:      "nokey",
			fields:    fields{},
			initError: true,
		},
		{
			name: "not exist key",
			fields: fields{
				JsonContentKey: "content",
			},
			initError:      false,
			content:        "json",
			key:            "con",
			logsLen:        1,
			logsContentLen: 1,
			validator: func(log *protocol.Log, t *testing.T) {
				require.Equal(t, test.ReadLogVal(log, "con"), "json")
				require.Equal(t, test.ReadLogVal(log, "content"), "")
			},
		},
		{
			name: "content val illegal",
			fields: fields{
				JsonContentKey: "content",
			},
			initError:      false,
			content:        "json",
			key:            "content",
			logsLen:        1,
			logsContentLen: 1,
			validator: func(log *protocol.Log, t *testing.T) {
				require.Equal(t, test.ReadLogVal(log, "content"), "json")
			},
		},
		{
			name: "not content path",
			fields: fields{
				JsonContentKey: "content",
			},
			initError:      false,
			content:        `{"a":"b"}`,
			key:            "content",
			logsLen:        1,
			logsContentLen: 1,
			validator: func(log *protocol.Log, t *testing.T) {
				require.Equal(t, test.ReadLogVal(log, "content"), `{"__instance_id__":"id_xxx","__instance_name__":"name_xxx","__instance_tags___tag_key":"tag_val","a":"b"}`)
			},
		},
		{
			name: "path type illegal",
			fields: fields{
				JsonContentKey:  "content",
				JsonContentPath: "a",
			},
			initError:      false,
			content:        `{"a":"b"}`,
			key:            "content",
			logsLen:        1,
			logsContentLen: 1,
			validator: func(log *protocol.Log, t *testing.T) {
				require.Equal(t, test.ReadLogVal(log, "content"), `{"a":"b"}`)
			},
		},
		{
			name: "path type illegal2",
			fields: fields{
				JsonContentKey:  "content",
				JsonContentPath: "a.b.c",
			},
			initError:      false,
			content:        `{"a": { "b": {"c": "d"}}}`,
			key:            "content",
			logsLen:        1,
			logsContentLen: 1,
			validator: func(log *protocol.Log, t *testing.T) {
				require.Equal(t, test.ReadLogVal(log, "content"), `{"a": { "b": {"c": "d"}}}`)
			},
		},
		{
			name: "path type legal",
			fields: fields{
				JsonContentKey:  "content",
				JsonContentPath: "a.b.c",
			},
			initError:      false,
			content:        `{"a": { "b": {"c": {"d":"e"}}}}`,
			key:            "content",
			logsLen:        1,
			logsContentLen: 1,
			validator: func(log *protocol.Log, t *testing.T) {
				require.Equal(t, test.ReadLogVal(log, "content"), `{"a":{"b":{"c":{"__instance_id__":"id_xxx","__instance_name__":"name_xxx","__instance_tags___tag_key":"tag_val","d":"e"}}}}`)
			},
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			c := new(ProcessorCloudMeta)
			c.Platform = platformmeta.Mock
			c.Mode = contentJsonMode
			c.AddMetas = metas
			c.JsonContentKey = tt.fields.JsonContentKey
			c.JsonContentPath = tt.fields.JsonContentPath
			if tt.initError {
				require.Error(t, c.Init(mock.NewEmptyContext("a", "b", "c")))
				return
			} else {
				require.NoError(t, c.Init(mock.NewEmptyContext("a", "b", "c")))
			}
			log := &protocol.Log{
				Time: 1,
				Contents: []*protocol.Log_Content{
					{
						Key:   tt.key,
						Value: tt.content,
					},
				},
			}
			logs := c.ProcessLogs([]*protocol.Log{log})
			require.Equal(t, len(logs), tt.logsLen)
			require.Equal(t, len(logs[0].Contents), tt.logsContentLen)
			tt.validator(logs[0], t)
		})
	}

}
