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

var insertSQL = "INSERT INTO `%s`.`ilogtail_%s_buffer` (_timestamp, _log) VALUES (?, ?)"

type FlusherClickHouse struct {
	context ilogtail.Context
	conn    driver.Conn

	// log configuration
	KeyValuePairs bool

	// ClickHouse configuration
	// Whether to print debug logs, default false
	Debug bool
	// Required parameters
	Addrs []string
	// Authentication using PLAIN
	Authentication Authentication
	// Cluster ClickHouse cluster name
	Cluster string
	// Table Target of data writing
	Table string
	// Data compression strategy
	Compression string
	// Timeout duration of a single request, unit: second , the default is  60
	MaxExecutionTime int
	// Dial timeout, the default is 10s
	DialTimeout time.Duration
	// Maximum number of connections, including long and short connections, the default is 5
	MaxOpenConns int
	// The maximum number of idle connections, that is the size of the connection pool, the default is 5
	MaxIdleConns int
	// Connection maximum lifetime, the default is 10m
	ConnMaxLifetime time.Duration
	// BlockBufferSize, size of block buffer, the default is 10
	BlockBufferSize uint8

	// ClickHouse Buffer engine configuration
	// The number of buffer, recommended as 16
	BufferNumLayers int
	// Data is flushed to disk when all min conditions are met, or when one max condition is met
	BufferMinTime  int
	BufferMaxTime  int
	BufferMinRows  int
	BufferMaxRows  int
	BufferMinBytes int
	BufferMaxBytes int

	flusher FlusherFunc
}

type FlusherFunc func(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error

func NewFlusherClickHouse() *FlusherClickHouse {
	return &FlusherClickHouse{
		Debug: false,
		Addrs: []string{},
		Authentication: Authentication{
			PlainText: &PlainTextConfig{
				Username: "",
				Password: "",
				Database: "",
			},
		},
		Cluster:          "",
		Table:            "",
		Compression:      "lz4",
		MaxExecutionTime: 60,
		DialTimeout:      time.Duration(10) * time.Second,
		MaxOpenConns:     5,
		MaxIdleConns:     5,
		ConnMaxLifetime:  time.Duration(10) * time.Minute,
		BlockBufferSize:  10,
		KeyValuePairs:    true,
		BufferNumLayers:  16,
		BufferMinTime:    10,
		BufferMaxTime:    100,
		BufferMinRows:    10000,
		BufferMaxRows:    1000000,
		BufferMinBytes:   10000000,
		BufferMaxBytes:   100000000,
	}
}

func (f *FlusherClickHouse) Init(ctx ilogtail.Context) error {
	f.context = ctx
	if f.Addrs == nil || len(f.Addrs) == 0 {
		var err = fmt.Errorf("clickhouse addrs is nil")
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}
	if f.Table == "" {
		var err = fmt.Errorf("clickhouse table is nil")
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}
	conn, err := newConn(f)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}
	f.conn = conn
	if err = createNullBufferTable(f); err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}
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
	sql := fmt.Sprintf(insertSQL, f.Authentication.PlainText.Database, f.Table)
	// post them to db all at once
	// build statements
	batch, err := f.conn.PrepareBatch(ctx, sql)
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
	return f.conn != nil
}

func (f *FlusherClickHouse) Stop() error {
	return f.conn.Close()
}

func init() {
	ilogtail.Flushers["flusher_clickhouse"] = func() ilogtail.Flusher {
		f := NewFlusherClickHouse()
		f.flusher = f.BufferFlush
		return f
	}
}

func newConn(f *FlusherClickHouse) (driver.Conn, error) {
	compression, err := compressionMethod(f.Compression)
	if err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return nil, err
	}
	opt := &clickhouse.Options{ // #nosec G402
		TLS: &tls.Config{
			InsecureSkipVerify: true,
		},
		Addr: f.Addrs,
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
			Method: compression,
		},
		DialTimeout:      f.DialTimeout,
		MaxOpenConns:     f.MaxOpenConns,
		MaxIdleConns:     f.MaxIdleConns,
		ConnMaxLifetime:  f.ConnMaxLifetime,
		ConnOpenStrategy: clickhouse.ConnOpenInOrder,
		BlockBufferSize:  f.BlockBufferSize,
	}
	if err = f.Authentication.ConfigureAuthentication(opt); err != nil {
		err = fmt.Errorf("configure authenticationfailed, err: %w", err)
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return nil, err
	}
	conn, err := clickhouse.Open(opt)
	if err != nil {
		err = fmt.Errorf("sql open failed, err: %w", err)
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return nil, err
	}
	if err = conn.Ping(context.Background()); err != nil {
		if exception, ok := err.(*clickhouse.Exception); ok {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "code", exception.Code, "msg", exception.Message, "trace", exception.StackTrace)
		} else {
			logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "error", err)
		}
		return nil, err
	}
	return conn, nil
}

func createNullBufferTable(f *FlusherClickHouse) error {
	sqlNullTableName := fmt.Sprintf("`%s`.`ilogtail_%s`", f.Authentication.PlainText.Database, f.Table)
	sqlBufferTableName := fmt.Sprintf("`%s`.`ilogtail_%s_buffer`", f.Authentication.PlainText.Database, f.Table)
	if f.Cluster != "" {
		sqlNullTableName = fmt.Sprintf("%s on cluster '%s'", sqlNullTableName, f.Cluster)
		sqlBufferTableName = fmt.Sprintf("%s on cluster '%s'", sqlBufferTableName, f.Cluster)
	}
	sqlNull := fmt.Sprintf("CREATE TABLE IF NOT EXISTS %s (`_timestamp` Int64,`_log` String) ENGINE = Null", sqlNullTableName)
	if err := f.conn.Exec(context.Background(), sqlNull); err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "sql", sqlNull, "error", err)
		return err
	}
	sqlBuffer := fmt.Sprintf("CREATE TABLE IF NOT EXISTS %s AS `%s`.`ilogtail_%s` ENGINE = Buffer(%s, ilogtail_%s, %d, %d, %d, %d, %d, %d, %d)",
		sqlBufferTableName,
		f.Authentication.PlainText.Database, f.Table,
		f.Authentication.PlainText.Database, f.Table,
		f.BufferNumLayers, f.BufferMinTime, f.BufferMaxTime, f.BufferMinRows, f.BufferMaxRows, f.BufferMinBytes, f.BufferMaxBytes,
	)
	if err := f.conn.Exec(context.Background(), sqlBuffer); err != nil {
		logger.Error(f.context.GetRuntimeContext(), "FLUSHER_INIT_ALARM", "sql", sqlBuffer, "error", err)
		return err
	}
	return nil
}

func compressionMethod(compression string) (clickhouse.CompressionMethod, error) {
	switch compression {
	case "none":
		return clickhouse.CompressionNone, nil
	case "gzip":
		return clickhouse.CompressionGZIP, nil
	case "lz4":
		return clickhouse.CompressionLZ4, nil
	case "zstd":
		return clickhouse.CompressionZSTD, nil
	case "deflate":
		return clickhouse.CompressionDeflate, nil
	case "br":
		return clickhouse.CompressionBrotli, nil
	default:
		return clickhouse.CompressionNone, fmt.Errorf("producer.compression should be one of 'none', 'gzip', 'deflate', 'lz4', 'br' or 'zstd'. configured value %v", compression)
	}
}
