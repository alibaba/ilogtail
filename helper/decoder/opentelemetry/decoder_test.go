// Copyright 2021 iLogtail Authors
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

package opentelemetry

import (
	"fmt"
	"net/http"
	"testing"

	"github.com/stretchr/testify/assert"
)

var textFormat = `{"resourceMetrics":[{"resource":{"attributes":[{"key":"service.name","value":{"stringValue":"unknown_service:ilogtail.test"}},{"key":"telemetry.sdk.language","value":{"stringValue":"go"}},{"key":"telemetry.sdk.name","value":{"stringValue":"opentelemetry"}},{"key":"telemetry.sdk.version","value":{"stringValue":"1.10.0"}}]},"scopeMetrics":[{"scope":{"name":"go.opentelemetry.io/otel/exporters/otlp/otlpmetric/otlpmetricgrpc_test"},"metrics":[{"name":"an_important_metric","description":"Measures the cumulative epicness of the app","sum":{"dataPoints":[{"startTimeUnixNano":"1663059119009537363","timeUnixNano":"1663059119009885985","asDouble":10}],"aggregationTemporality":"AGGREGATION_TEMPORALITY_CUMULATIVE","isMonotonic":true}}]}],"schemaUrl":"https://opentelemetry.io/schemas/1.12.0"}]}`

func TestNormal(t *testing.T) {
	httpReq, _ := http.NewRequest("Post", "", nil)
	httpReq.Header.Set("Content-Type", jsonContentType)
	decoder := &Decoder{}

	// otlp_req := pmetricotlp.NewRequest()
	// data, err := otlp_req.MarshalJSON()
	logs, err := decoder.Decode([]byte(textFormat), httpReq)
	fmt.Println(logs)
	assert.Nil(t, err)
}
