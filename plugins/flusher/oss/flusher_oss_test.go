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

package oss

import (
	"strconv"
	"testing"

	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/plugins/test"
	"github.com/alibaba/ilogtail/plugins/test/mock"
	"github.com/stretchr/testify/require"
)

func TestConnectAndWrite(t *testing.T) {
	f := initOssClient()
	lgl := makeTestLogGroupList()
	err := f.Flush("projectName", "logstoreName", "configName", lgl.GetLogGroupList())
	require.NoError(t, err)
	_ = f.Stop()
}

func TestPath(t *testing.T) {
	f := initOssClient()
	f.KeyFormat = "ilogtail1/%{ilogtail.hostname}/%{+yyyy}/%{+MM}/%{+dd}/var/log/%{content.filename}"
	lgl := makeTestLogGroupList()
	err := f.Flush("projectName", "logstoreName", "configName", lgl.GetLogGroupList())
	require.NoError(t, err)
	_ = f.Stop()
}

func TestConfigs(t *testing.T) {
	f := initOssClient()
	f.KeyFormat = "ilogtail2/%{ilogtail.hostname}/%{+yyyy.MM.dd}/var/log/%{content.filename}"
	f.Encoding = "gzip"
	f.ObjectAcl = "private"
	f.ObjectStorageClass = "IA"
	f.Tagging = "TagA=A&TagB=B"
	lgl := makeTestLogGroupList()
	err := f.Flush("projectName", "logstoreName", "configName", lgl.GetLogGroupList())
	require.NoError(t, err)
	_ = f.Stop()
}

func initOssClient() *FlusherOss {
	f := NewFlusherOss()
	f.Endpoint = "oss-cn-hangzhou.aliyuncs.com"
	f.Bucket = "jinchen-test"
	f.KeyFormat = "ilogtail/%{content.hostid}/var/log/%{content.filename}"
	// f.MaximumFileSize = 500
	f.ContentKey = "message"
	lctx := mock.NewEmptyContext("p", "l", "c")
	f.Init(lctx)
	return f
}

func makeTestLogGroupList() *protocol.LogGroupList {
	f := map[string]string{}
	lgl := &protocol.LogGroupList{
		LogGroupList: make([]*protocol.LogGroup, 0, 10),
	}
	f["filename"] = "messages"
	for i := 1; i <= 10; i++ {
		lg := &protocol.LogGroup{
			Logs: make([]*protocol.Log, 0, 10),
		}
		for j := 1; j <= 10; j++ {
			f["hostid"] = "host-" + strconv.Itoa(i%2)
			f["group"] = strconv.Itoa(i)
			f["message"] = "The message: " + strconv.Itoa(j)
			l := test.CreateLogByFields(f)
			lg.Logs = append(lg.Logs, l)
		}
		lgl.LogGroupList = append(lgl.LogGroupList, lg)
	}
	return lgl
}
