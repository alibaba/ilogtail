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

import (
	"errors"
	"io"
	"net/url"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestIsEOF(t *testing.T) {
	err := &url.Error{
		Op:  "Post",
		URL: "http://test",
		Err: io.EOF,
	}
	assert.True(t, IsErrorEOF(err))

	err = &url.Error{
		Op:  "Post",
		URL: "http://test",
		Err: errors.New("connection reset by peer"),
	}
	assert.True(t, IsErrorEOF(err))
}
