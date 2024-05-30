package verify

import (
	"context"
	"fmt"
	"strconv"
	"time"

	"github.com/alibaba/ilogtail/test/config"
	"github.com/alibaba/ilogtail/test/testhub/control"
	sls "github.com/alibabacloud-go/sls-20201230/v5/client"
	"github.com/avast/retry-go/v4"
)

const queryCountSql = "* | SELECT COUNT(1) as count FROM log WHERE from_unixtime(__time__) >= from_unixtime(%v) AND from_unixtime(__time__) < now()"

func VerifyLogCount(ctx context.Context, expect int) (context.Context, error) {
	var from int32
	value := ctx.Value("startTime")
	if value != nil {
		from = value.(int32)
	} else {
		return ctx, fmt.Errorf("no start time")
	}
	sql := fmt.Sprintf(queryCountSql, from)

	timeoutCtx, cancel := context.WithTimeout(context.TODO(), config.TestConfig.RetryTimeout)
	defer cancel()
	var resp *sls.GetLogsResponse
	var err error
	err = retry.Do(
		func() error {
			resp, err = control.GetLogFromSLS(sql, from)
			if err != nil {
				return err
			}
			var count int
			count, err = strconv.Atoi(resp.Body[0]["count"].(string))
			if err != nil {
				return err
			}
			if count != expect {
				err = fmt.Errorf("log count not match, expect %d, got %d", expect, count)
				return err
			}
			return nil
		},
		retry.Context(timeoutCtx),
		retry.Delay(5*time.Second),
		retry.DelayType(retry.FixedDelay),
	)
	if err != nil {
		return ctx, err
	}
	return ctx, nil
}
