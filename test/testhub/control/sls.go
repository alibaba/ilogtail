package control

import (
	"fmt"

	"github.com/alibaba/ilogtail/test/config"
	openapi "github.com/alibabacloud-go/darabonba-openapi/v2/client"
	sls "github.com/alibabacloud-go/sls-20201230/v5/client"
	"github.com/alibabacloud-go/tea/tea"
)

var slsClient *sls.Client

func GetLogFromSLS(sql string, from, to int32) (*sls.GetLogsResponse, error) {
	req := &sls.GetLogsRequest{
		Query: tea.String(sql),
		From:  tea.Int32(from),
		To:    tea.Int32(to),
	}
	resp, err := slsClient.GetLogs(tea.String(config.TestConfig.Project), tea.String(config.TestConfig.Logstore), req)
	if err != nil {
		return nil, fmt.Errorf("failed to get logs with sql %s, %v", sql, err)
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

func init() {
	slsClient = createSLSClient(config.TestConfig.AccessKeyId, config.TestConfig.AccessKeySecret, config.TestConfig.Endpoint)
}
