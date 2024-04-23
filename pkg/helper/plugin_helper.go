// Copyright 2024 iLogtail Authors
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

package helper

import "strings"

const delimiter = '|'

func ConvertCreatorMapToString[T any](buffer *strings.Builder, s map[string]T) {
	for k := range s {
		if buffer.Len() > 0 {
			buffer.WriteRune(delimiter)
		}
		buffer.WriteString(k)
	}
}
