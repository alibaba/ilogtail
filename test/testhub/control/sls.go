package control

import (
	"fmt"
	"sync"
	"time"

	"github.com/alibaba/ilogtail/test/config"
	openapi "github.com/alibabacloud-go/darabonba-openapi/v2/client"
	sls "github.com/alibabacloud-go/sls-20201230/v5/client"
	"github.com/alibabacloud-go/tea/tea"
)

var slsClient *sls.Client
var slsClientOnce sync.Once

func GetLogFromSLS(sql string, from int32) (*sls.GetLogsResponse, error) {
	slsClientOnce.Do(func() {
		slsClient = createSLSClient(config.TestConfig.AccessKeyId, config.TestConfig.AccessKeySecret, config.TestConfig.QueryEndpoint)
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

func createSLSClient(accessKeyId, accessKeySecret, endpoint string) *sls.Client {
	config := &openapi.Config{
		AccessKeyId:     tea.String(accessKeyId),
		AccessKeySecret: tea.String(accessKeySecret),
		Endpoint:        tea.String(endpoint),
	}
	client, _ := sls.NewClient(config)
	return client
}
