// Copyright 2023 iLogtail Authors
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

package otlp

import (
	"strings"

	"go.opentelemetry.io/collector/pdata/plog"
)

func SeverityTextToSeverityNumber(severityText string) plog.SeverityNumber {
	severity := strings.ToLower(severityText)

	switch severity {
	case "trace":
		return plog.SeverityNumberTrace
	case "trace2":
		return plog.SeverityNumberTrace2
	case "trace3":
		return plog.SeverityNumberTrace3
	case "trace4":
		return plog.SeverityNumberTrace4
	case "debug":
		return plog.SeverityNumberDebug
	case "debug2":
		return plog.SeverityNumberDebug2
	case "debug3":
		return plog.SeverityNumberDebug3
	case "debug4":
		return plog.SeverityNumberDebug4
	case "info", "information":
		return plog.SeverityNumberInfo
	case "info2", "information2":
		return plog.SeverityNumberInfo2
	case "info3", "information3":
		return plog.SeverityNumberInfo3
	case "info4", "information4":
		return plog.SeverityNumberInfo4
	case "warn", "warning":
		return plog.SeverityNumberWarn
	case "warn2", "warning2":
		return plog.SeverityNumberWarn2
	case "warn3", "warning3":
		return plog.SeverityNumberWarn3
	case "warn4", "warning4":
		return plog.SeverityNumberWarn4
	case "error":
		return plog.SeverityNumberError
	case "error2":
		return plog.SeverityNumberError2
	case "error3":
		return plog.SeverityNumberError3
	case "error4":
		return plog.SeverityNumberError4
	case "fatal":
		return plog.SeverityNumberFatal
	case "fatal2":
		return plog.SeverityNumberFatal2
	case "fatal3":
		return plog.SeverityNumberFatal3
	case "fatal4":
		return plog.SeverityNumberFatal4
	default:
		return plog.SeverityNumberInfo
	}
}
