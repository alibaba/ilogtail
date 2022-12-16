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

package clickhouse

import (
	"context"
	"crypto/tls"
	"encoding/json"
	"fmt"
	"net"
	"strconv"
	"time"

	"github.com/ClickHouse/clickhouse-go/v2"
	"github.com/ClickHouse/clickhouse-go/v2/lib/driver"
	jsoniter "github.com/json-iterator/go"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

var insertSQL = "INSERT INTO `%s`.`%s` (_timestamp, _log) VALUES (?, ?)"

type FlusherClickHouse struct {
	context ilogtail.Context
	client  driver.Conn
	flusher FlusherFunc
	// Required parameters
	Addrs    []string
	User     string
	Password string
	Database string
	Table    string
	// Whether to print debug logs, default false
	Debug bool
	// Timeout duration of a single request, unit: second
	// default 60
	MaxExecutionTime int
	// Dial timeout, default 10s
	DialTimeout time.Duration
	// Maximum number of connections, including long and short connections
	// default 5
	MaxOpenConns int
	// The maximum number of idle connections, that is the size of the connection pool
	// default 5
	MaxIdleConns int
	// Connection maximum lifetime
	// default 10m
	ConnMaxLifetime time.Duration
	BlockBufferSize uint8
	KeyValuePairs   bool
}

type FlusherFunc func(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error

func NewFlusherClickHouse() *FlusherClickHouse {
	return &FlusherClickHouse{
		context:          nil,
		client:           nil,
		Addrs:            []string{},
		User:             "",
		Password:         "",
		Database:         "",
		Table:            "",
		Debug:            false,
		MaxExecutionTime: 60,
		DialTimeout:      time.Duration(10) * time.Second,
		MaxOpenConns:     5,
		MaxIdleConns:     5,
		ConnMaxLifetime:  time.Duration(10) * time.Minute,
		BlockBufferSize:  10,
		KeyValuePairs:    true,
	}
}

func (f *FlusherClickHouse) Init(ctx ilogtail.Context) error {
	f.context = ctx
	if len(f.Addrs) == 0 {
		var err = fmt.Errorf("clickhouse addrs is nil")
		logger.Error(f.context.GetRuntimeContext(), "CLICKHOUSE_FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}
	if f.Database == "" {
		var err = fmt.Errorf("clickhouse database is nil")
		logger.Error(f.context.GetRuntimeContext(), "CLICKHOUSE_FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}
	if f.Table == "" {
		var err = fmt.Errorf("clickhouse table is nil")
		logger.Error(f.context.GetRuntimeContext(), "CLICKHOUSE_FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}
	db, err := clickhouse.Open(&clickhouse.Options{
		TLS:  &tls.Config{InsecureSkipVerify: true},
		Addr: f.Addrs,
		Auth: clickhouse.Auth{
			Database: f.Database,
			Username: f.User,
			Password: f.Password,
		},
		DialContext: func(ctx context.Context, addr string) (net.Conn, error) {
			var d net.Dialer
			return d.DialContext(ctx, "tcp", addr)
		},
		Debug: f.Debug,
		Debugf: func(format string, v ...interface{}) {
			fmt.Printf(format, v)
		},
		Settings: clickhouse.Settings{
			"max_execution_time": f.MaxExecutionTime,
		},
		Compression: &clickhouse.Compression{
			Method: clickhouse.CompressionLZ4,
		},
		DialTimeout:      f.DialTimeout,
		MaxOpenConns:     f.MaxOpenConns,
		MaxIdleConns:     f.MaxIdleConns,
		ConnMaxLifetime:  f.ConnMaxLifetime,
		ConnOpenStrategy: clickhouse.ConnOpenInOrder,
		BlockBufferSize:  f.BlockBufferSize,
	})
	if err != nil {
		err = fmt.Errorf("sql open failed, err: %w", err)
		logger.Error(f.context.GetRuntimeContext(), "CLICKHOUSE_FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}
	if err = db.Ping(context.Background()); err != nil {
		if exception, ok := err.(*clickhouse.Exception); ok {
			logger.Error(f.context.GetRuntimeContext(), "CLICKHOUSE_FLUSHER_INIT_ALARM", "code", exception.Code, "msg", exception.Message, "trace", exception.StackTrace)
		} else {
			logger.Error(f.context.GetRuntimeContext(), "CLICKHOUSE_FLUSHER_INIT_ALARM", "error", err)
		}
		return err
	}
	f.client = db
	return nil
}

func (f *FlusherClickHouse) Description() string {
	return "ClickHouse flusher for logtail"
}

func (f *FlusherClickHouse) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	if f.flusher == nil {
		return fmt.Errorf("clickHouse flusher func must be configured")
	}
	return f.flusher(projectName, logstoreName, configName, logGroupList)
}

func (f *FlusherClickHouse) BufferFlush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	ctx := context.Background()
	sql := fmt.Sprintf(insertSQL, f.Database, f.Table)
	// post them to db all at once
	// build statements
	batch, err := f.client.PrepareBatch(ctx, sql)
	if err != nil {
		return err
	}
	for _, logGroup := range logGroupList {
		if f.KeyValuePairs {
			for _, log := range logGroup.Logs {
				writer := jsoniter.NewStream(jsoniter.ConfigDefault, nil, 128)
				writer.WriteObjectStart()
				for _, c := range log.Contents {
					writer.WriteObjectField(c.Key)
					writer.WriteString(c.Value)
					_, _ = writer.Write([]byte{','})
				}
				writer.WriteObjectField("_timestamp")
				writer.WriteString(strconv.Itoa(int(log.Time)))
				writer.WriteObjectEnd()
				if err = batch.Append(int64(log.Time), string(writer.Buffer())); err != nil {
					return err
				}
			}
		} else {
			logger.Debug(f.context.GetRuntimeContext(), "[LogGroup] topic", logGroup.Topic, "logstore", logGroup.Category, "logcount", len(logGroup.Logs), "tags", logGroup.LogTags)
			for _, log := range logGroup.Logs {
				buf, _ := json.Marshal(log)
				logger.Debug(f.context.GetRuntimeContext(), string(buf))
				if err = batch.Append(int64(log.Time), string(buf)); err != nil {
					return err
				}
			}
		}
	}
	// commit and record metrics
	return batch.Send()
}

func (f *FlusherClickHouse) SetUrgent(flag bool) {}

func (f *FlusherClickHouse) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	return f.client != nil
}

func (f *FlusherClickHouse) Stop() error {
	return f.client.Close()
}

func init() {
	ilogtail.Flushers["flusher_clickhouse"] = func() ilogtail.Flusher {
		f := NewFlusherClickHouse()
		f.flusher = f.BufferFlush
		return f
	}
}
