// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package helper

import (
	"testing"

	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/stretchr/testify/assert"
)

func TestStrMetricV2_Name(t *testing.T) {
	type fields struct {
		name   string
		value  string
		labels []*protocol.Log_Content
	}
	tests := []struct {
		name   string
		fields fields
		want   string
	}{
		{
			name: "test_name",
			fields: fields{
				name:  "field",
				value: "v",
			},
			want: "field",
		},
		{
			name: "test_name",
			fields: fields{
				name:  "field",
				value: "v",
				labels: []*protocol.Log_Content{
					{
						Key:   "key",
						Value: "value",
					},
				},
			},
			want: "field",
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			constLabels := map[string]string{}
			for _, v := range tt.fields.labels {
				constLabels[v.Key] = v.Value
			}
			metric := NewStringMetricVector(tt.fields.name, constLabels, nil).WithLabels()
			err := metric.Set(tt.fields.value)
			assert.NoError(t, err)

			if got := metric.(*strMetricImp).Name(); got != tt.want {
				t.Errorf("StrMetric.Name() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestStrMetricV2_Set(t *testing.T) {
	type fields struct {
		name  string
		value string
	}
	type args struct {
		v string
	}
	tests := []struct {
		name   string
		fields fields
		args   args
	}{
		{
			name: "1",
			fields: fields{
				name:  "n",
				value: "v",
			},
			args: args{
				"x",
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			constLabels := map[string]string{}
			s := NewStringMetricVector(tt.fields.name, constLabels, nil).WithLabels()
			err := s.Set(tt.args.v)
			assert.NoError(t, err)
			if s.Get().Value != tt.args.v {
				t.Errorf("fail %s != %s\n", s.Get(), tt.args.v)
			}
		})
	}
}
