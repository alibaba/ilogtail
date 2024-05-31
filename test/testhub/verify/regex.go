package verify

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/test/config"
	"github.com/alibaba/ilogtail/test/testhub/control"
	"github.com/avast/retry-go/v4"
)

const queryRegexSQL = "* | SELECT %s FROM log WHERE from_unixtime(__time__) >= from_unixtime(%v) AND from_unixtime(__time__) < now()"

func RegexSingle(ctx context.Context) (context.Context, error) {
	var from int32
	value := ctx.Value(config.StartTimeContextKey)
	if value != nil {
		from = value.(int32)
	} else {
		return ctx, fmt.Errorf("no start time")
	}
	fields := []string{"mark", "file", "logNo", "ip", "time", "method", "url", "http", "status", "size", "userAgent", "msg"}
	sql := fmt.Sprintf(queryRegexSQL, strings.Join(fields, ", "), from)
	timeoutCtx, cancel := context.WithTimeout(context.TODO(), config.TestConfig.RetryTimeout)
	defer cancel()
	var err error
	err = retry.Do(
		func() error {
			if _, err = control.GetLogFromSLS(sql, from); err != nil {
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
