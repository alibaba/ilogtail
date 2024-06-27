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
package verify

import (
	"context"
	"fmt"
	"strconv"
	"time"

	sls "github.com/alibabacloud-go/sls-20201230/v5/client"
	"github.com/avast/retry-go/v4"

	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/test/config"
	"github.com/alibaba/ilogtail/test/engine/control"
	"github.com/alibaba/ilogtail/test/engine/setup/subscriber"
)

const queryCountSQL = "* | SELECT COUNT(1) as count FROM log WHERE from_unixtime(__time__) >= from_unixtime(%v) AND from_unixtime(__time__) < now()"

func LogCount(ctx context.Context, expect int) (context.Context, error) {
	var from int32
	value := ctx.Value(config.StartTimeContextKey)
	if value != nil {
		from = value.(int32)
	} else {
		return ctx, fmt.Errorf("no start time")
	}
	sql := fmt.Sprintf(queryCountSQL, from)

	timeoutCtx, cancel := context.WithTimeout(context.TODO(), config.TestConfig.RetryTimeout)
	defer cancel()
	var resp *sls.GetLogsResponse
	var err error
	var count int
	err = retry.Do(
		func() error {
			resp, err = control.GetLogFromSLS(sql, from)
			if err != nil {
				return err
			}
			count, err = strconv.Atoi(resp.Body[0]["count"].(string))
			if err != nil {
				return err
			}
			if count != expect {
				return fmt.Errorf("log count not match, expect %d, got %d", expect, count)
			}
			if expect == 0 {
				return fmt.Errorf("log count is 0")
			}
			return nil
		},
		retry.Context(timeoutCtx),
		retry.Delay(5*time.Second),
		retry.DelayType(retry.FixedDelay),
	)
	if expect == 0 && count == expect {
		return ctx, nil
	}
	if err != nil {
		return ctx, err
	}
	return ctx, nil
}

func LogCountAtLeast(ctx context.Context, expect int) (context.Context, error) {
	var from int32
	value := ctx.Value(config.StartTimeContextKey)
	if value != nil {
		from = value.(int32)
	} else {
		return ctx, fmt.Errorf("no start time")
	}
	timeoutCtx, cancel := context.WithTimeout(context.TODO(), config.TestConfig.RetryTimeout)
	defer cancel()
	var groups []*protocol.LogGroup
	var err error
	var count int
	err = retry.Do(
		func() error {
			groups, err = subscriber.TestSubscriber.GetData(from)
			if err != nil {
				return err
			}
			for _, group := range groups {
				count += len(group.Logs)
			}
			if count < expect {
				return fmt.Errorf("log count not match, expect at least %d, got %d", expect, count)
			}
			if expect == 0 {
				return fmt.Errorf("log count is 0")
			}
			return nil
		},
		retry.Context(timeoutCtx),
		retry.Delay(5*time.Second),
		retry.DelayType(retry.FixedDelay),
	)
	if expect == 0 && count == expect {
		return ctx, nil
	}
	if err != nil {
		return ctx, err
	}
	return ctx, nil
}

func LogCountAtLeastWithFilter(ctx context.Context, expect int, filterKey string, filterValue string) (context.Context, error) {
	var from int32
	value := ctx.Value(config.StartTimeContextKey)
	if value != nil {
		from = value.(int32)
	} else {
		return ctx, fmt.Errorf("no start time")
	}
	timeoutCtx, cancel := context.WithTimeout(context.TODO(), config.TestConfig.RetryTimeout)
	defer cancel()
	var groups []*protocol.LogGroup
	var err error
	var count int
	err = retry.Do(
		func() error {
			groups, err = subscriber.TestSubscriber.GetData(from)
			if err != nil {
				return err
			}
			count = 0
			for _, group := range groups {
				for _, log := range group.Logs {
					for _, content := range log.Contents {
						if content.Key == filterKey && content.Value == filterValue {
							count++
						}
					}
				}
			}
			if count < expect {
				return fmt.Errorf("log count not match, expect at least %d, got %d", expect, count)
			}
			if expect == 0 {
				return fmt.Errorf("log count is 0")
			}
			return nil
		},
		retry.Context(timeoutCtx),
		retry.Delay(5*time.Second),
		retry.DelayType(retry.FixedDelay),
	)
	if expect == 0 && count == expect {
		return ctx, nil
	}
	if err != nil {
		return ctx, err
	}
	return ctx, nil
}
