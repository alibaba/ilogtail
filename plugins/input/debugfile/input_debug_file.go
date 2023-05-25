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

package debugfile

import (
	"io"
	"os"

	"github.com/alibaba/ilogtail/pkg/pipeline"
)

// InputDebugFile can reads all data in specified file, then split them by \n and set them as same field.
type InputDebugFile struct {
	InputFilePath string
	FieldName     string
	LineLimit     int

	context pipeline.Context
	logs    []string
}

// Init ...
func (r *InputDebugFile) Init(context pipeline.Context) (int, error) {
	r.context = context
	file, err := os.Open(r.InputFilePath)
	if err != nil {
		return 0, err
	}
	defer file.Close()

	buff := make([]byte, 0, 4096)
	char := make([]byte, 1)

	// 查询文件大小
	stat, _ := file.Stat()
	filesize := stat.Size()

	var cursor int64 = 0
	cnt := 0
	for {
		cursor -= 1
		_, _ = file.Seek(cursor, io.SeekEnd)
		_, err := file.Read(char)
		if err != nil {
			panic(err)
		}

		if char[0] == '\n' || cursor == -filesize {
			if cursor == -filesize {
				buff = append(buff, char[0])
			}
			if len(buff) > 0 {
				revers(buff)
				// 读取到的行
				r.logs = append(r.logs, string(buff))

				cnt++
				if cnt == r.LineLimit {
					// 超过数量退出
					break
				}

			}
			buff = buff[:0]
		} else {
			buff = append(buff, char[0])
		}

		if cursor == -filesize {
			break
		}
	}

	return 0, nil
}

// Description ...
func (r *InputDebugFile) Description() string {
	return "input plugin for debugging"
}

// Collect ...
func (r *InputDebugFile) Collect(collector pipeline.Collector) error {
	for i := len(r.logs) - 1; i >= 0; i-- {
		log := map[string]string{}
		log[r.FieldName] = r.logs[i]
		collector.AddData(nil, log)
	}

	return nil
}

func revers(s []byte) {
	for i, j := 0, len(s)-1; i < j; i, j = i+1, j-1 {
		s[i], s[j] = s[j], s[i]
	}
}

func init() {
	pipeline.MetricInputs["metric_debug_file"] = func() pipeline.MetricInput {
		return &InputDebugFile{
			FieldName: "content",
			LineLimit: 1000,
		}
	}
}
