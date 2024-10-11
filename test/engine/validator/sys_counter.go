// Copyright 2021 iLogtail Authors
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

package validator

import (
	"context"
	"strconv"

	"github.com/mitchellh/mapstructure"

	"github.com/alibaba/ilogtail/pkg/doc"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

const counterSysValidatorName = "sys_counter"

type counterSystemValidator struct {
	ExpectReceivedMinimumLogNum      int  `mapstructure:"expect_received_minimum_log_num"  comment:"the expected minimum received log number"`
	ExpectReceivedMinimumLogGroupNum int  `mapstructure:"expect_received_minimum_log_group_num"  comment:"the expected minimum received log group number"`
	ExpectMinimumRawLogNum           int  `mapstructure:"expect_minimum_raw_log_num"   comment:"the expected minimum raw log number"`
	ExpectMinimumProcessedLogNum     int  `mapstructure:"expect_minimum_processed_log_num"  comment:"the expected minimum processed log number"`
	ExpectMinimumFlushLogNum         int  `mapstructure:"expect_minimum_flush_log_num"  comment:"the expected minimum flushed log number"`
	ExpectMinimumFlushLogGroupNum    int  `mapstructure:"expect_minimum_flush_log_group_num" comment:"the expected minimum flushed log group number"`
	ExpectEqualRawLog                bool `mapstructure:"expect_equal_raw_log" comment:"whether the received log number equal to the raw log number"`
	ExpectEqualProcessedLog          bool `mapstructure:"expect_equal_processed_log" comment:"whether the received log number equal to the processed log number"`
	ExpectEqualFlushLog              bool `mapstructure:"expect_equal_flush_log" comment:"whether the received log number equal to the flush log number"`

	groupCounter int
	logsCounter  int
}

func (c *counterSystemValidator) Description() string {
	return "this is a log field validator to check received log from subscriber"
}

func (c *counterSystemValidator) Name() string {
	return counterSysValidatorName
}

func (c *counterSystemValidator) Valid(group *protocol.LogGroup) {
	c.groupCounter++
	c.logsCounter += len(group.Logs)
}

func (c *counterSystemValidator) Start() error {
	return nil
}

func (c *counterSystemValidator) FetchResult() (res []*Report) {
	if c.ExpectReceivedMinimumLogGroupNum > c.groupCounter {
		logger.Debugf(context.Background(), "want log group number over %d, bug got %d", c.ExpectReceivedMinimumLogGroupNum, c.groupCounter)
		res = append(res, &Report{Validator: counterSysValidatorName, Name: "loggroup_minimum_number", Want: strconv.Itoa(c.ExpectReceivedMinimumLogGroupNum), Got: strconv.Itoa(c.groupCounter)})
	}
	if c.ExpectReceivedMinimumLogNum > c.logsCounter {
		logger.Debugf(context.Background(), "want log number over %d, bug got %d", c.ExpectReceivedMinimumLogNum, c.logsCounter)
		res = append(res, &Report{Validator: counterSysValidatorName, Name: "log_minimum_number", Want: strconv.Itoa(c.ExpectReceivedMinimumLogNum), Got: strconv.Itoa(c.logsCounter)})
	}
	return
}

func init() {
	RegisterSystemValidatorCreator(counterSysValidatorName, func(spec map[string]interface{}) (SystemValidator, error) {
		f := new(counterSystemValidator)
		err := mapstructure.Decode(spec, f)
		if err != nil {
			return nil, err
		}
		return f, nil
	})
	doc.Register("sys_validator", counterSysValidatorName, new(counterSystemValidator))
}
