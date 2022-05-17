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

package telegraf

import (
	"bytes"
	"context"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type Processor struct {
}

func (p *Processor) Process(fileBlock []byte, noChangeInterval time.Duration) int {
	nowIndex := 0
	processedCount := 0
	for nextIndex := bytes.IndexByte(fileBlock, '\n'); nextIndex >= 0; nextIndex = bytes.IndexByte(fileBlock[nowIndex:], '\n') {
		nextIndex += nowIndex

		originalLine := fileBlock[nowIndex : nextIndex+1]
		separator := bytes.IndexByte(originalLine, ' ')
		if separator == -1 {
			logger.Warning(context.Background(), "TELEGRAF_ALARM", "illegal log", string(originalLine))
			continue
		}
		timePart := originalLine[0:separator]
		thisLine := originalLine[separator+1:]
		separator = bytes.IndexByte(thisLine, ' ')
		if separator == -1 {
			logger.Warning(context.Background(), "TELEGRAF_ALARM", "illegal log", string(originalLine))
			continue
		}
		levelPart := thisLine[0:separator]
		thisLine = thisLine[separator+1:]
		switch levelPart[0] {
		case 'D':
			logger.Debug(context.Background(), "time", string(timePart), "log", string(thisLine))
		case 'I':
			logger.Info(context.Background(), "time", string(timePart), "log", string(thisLine))
		case 'W':
			logger.Warning(context.Background(), "TELEGRAF_ALARM", "time", string(timePart), "log", string(thisLine))
		case 'E':
			logger.Error(context.Background(), "TELEGRAF_ALARM", "time", string(timePart), "log", string(thisLine))
		default:
			logger.Warning(context.Background(), "TELEGRAF_ALARM", "illegal log", string(originalLine))
		}
		processedCount = nextIndex + 1
		nowIndex = nextIndex + 1
	}
	return processedCount
}
