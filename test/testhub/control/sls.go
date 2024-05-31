// Copyright 2024 iLogtail Authors
//
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
package control

import (
	"fmt"
	"sync"
	"time"

	openapi "github.com/alibabacloud-go/darabonba-openapi/v2/client"
	sls "github.com/alibabacloud-go/sls-20201230/v5/client"
	"github.com/alibabacloud-go/tea/tea"

	"github.com/alibaba/ilogtail/test/config"
)

var slsClient *sls.Client
var slsClientOnce sync.Once

func GetLogFromSLS(sql string, from int32) (*sls.GetLogsResponse, error) {
	slsClientOnce.Do(func() {
		slsClient = createSLSClient(config.TestConfig.AccessKeyID, config.TestConfig.AccessKeySecret, config.TestConfig.QueryEndpoint)
	})
	now := int32(time.Now().Unix())
	if now == from {
		now++
	}
	req := &sls.GetLogsRequest{
		Query: tea.String(sql),
		From:  tea.Int32(from),
		To:    tea.Int32(now),
	}
	resp, err := slsClient.GetLogs(tea.String(config.TestConfig.Project), tea.String(config.TestConfig.Logstore), req)
	if err != nil {
		return nil, err
	}
	if len(resp.Body) == 0 {
		return nil, fmt.Errorf("failed to get logs with sql %s, no log", sql)
	}
	return resp, nil
}

func createSLSClient(accessKeyID, accessKeySecret, endpoint string) *sls.Client {
	config := &openapi.Config{
		AccessKeyId:     tea.String(accessKeyID),
		AccessKeySecret: tea.String(accessKeySecret),
		Endpoint:        tea.String(endpoint),
	}
	client, _ := sls.NewClient(config)
	return client
}
