package control

import (
	"context"
	"fmt"
	"time"

	"github.com/alibaba/ilogtail/test/config"
	openapi "github.com/alibabacloud-go/darabonba-openapi/v2/client"
	sls "github.com/alibabacloud-go/sls-20201230/v5/client"
	"github.com/alibabacloud-go/tea/tea"
	"github.com/avast/retry-go/v4"
)

var slsClient *sls.Client

func InitSLSClient() {
	slsClient = createSLSClient(config.TestConfig.AccessKeyId, config.TestConfig.AccessKeySecret, config.TestConfig.Endpoint)
}

func WaitLog2SLS(from int32, timeout time.Duration) {
	sql := fmt.Sprintf("* | SELECT * FROM log WHERE from_unixtime(__time__) >= from_unixtime(%v) AND from_unixtime(__time__) < now()", from)
	ctx, cancel := context.WithTimeout(context.TODO(), timeout)
	defer cancel()
	isValid := false
	retry.Do(
		func() error {
			resp, err := GetLogFromSLS(sql, from)
			if err != nil {
				return err
			}
			if len(resp.Body) > 0 {
				isValid = true
				return nil
			}
			return fmt.Errorf("empty log")
		},
		retry.Context(ctx),
		retry.Delay(5*time.Second),
		retry.DelayType(retry.FixedDelay),
	)
	if !isValid {
		panic(fmt.Errorf("wait timeout from sls"))
	}
}

func GetLogFromSLS(sql string, from int32) (*sls.GetLogsResponse, error) {
	fmt.Println(sql)
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
