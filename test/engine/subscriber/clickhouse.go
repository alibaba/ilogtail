package subscriber

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"net"
	"strconv"
	"strings"
	"time"

	"github.com/ClickHouse/clickhouse-go/v2"
	"github.com/ClickHouse/clickhouse-go/v2/lib/driver"

	"github.com/alibaba/ilogtail/pkg/doc"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"

	"github.com/mitchellh/mapstructure"
)

const clickHouseName = "clickhouse"
const clickhouseQuerySQL = "select _timestamp,_log from `%s`.`ilogtail_%s_buffer` where _timestamp > %v order by _timestamp"

type ClickHouseSubscriber struct {
	Addr            string `mapstructure:"addr" comment:"the clickhouse host address"`
	Username        string `mapstructure:"username" comment:"the clickhouse username"`
	Password        string `mapstructure:"password" comment:"the clickhouse password"`
	Database        string `mapstructure:"database" comment:"the clickhouse database name to query from"`
	Table           string `mapstructure:"table" comment:"the clickhouse table name to query from"`
	QueryIntervalMs int    `mapstructure:"query_interval_ms" comment:"interval between queries select upserts records from clickhouse"`
	CreateTable     bool   `mapstructure:"create_db" comment:"if create the database, default is true"`

	client        driver.Conn
	lastTimestamp int64
	channel       chan *protocol.LogGroup
	stopCh        chan struct{}
}

type logContent struct {
	Index     string `json:"Index"`
	Name      string `json:"_name"`
	Timestamp string `json:"_timestamp"`
	Value     string `json:"_value"`
}

func (i *ClickHouseSubscriber) Name() string {
	return clickHouseName
}

func (i *ClickHouseSubscriber) Description() string {
	return "this's a clickhouse subscriber, which will query inserted records from clickhouse periodically."
}

func (i *ClickHouseSubscriber) Start() error {
	host, err := TryReplacePhysicalAddress(i.Addr)
	if err != nil {
		return err
	}
	host = strings.ReplaceAll(host, "http://", "")
	conn, err := clickhouse.Open(&clickhouse.Options{
		Addr: []string{host},
		Auth: clickhouse.Auth{
			Database: i.Database,
		},
		DialContext: func(ctx context.Context, addr string) (net.Conn, error) {
			var d net.Dialer
			return d.DialContext(ctx, "tcp", addr)
		},
		Settings: clickhouse.Settings{
			"max_execution_time": 60,
		},
		Compression: &clickhouse.Compression{
			Method: clickhouse.CompressionLZ4,
		},
		DialTimeout:      time.Duration(10) * time.Second,
		MaxOpenConns:     5,
		MaxIdleConns:     5,
		ConnMaxLifetime:  time.Duration(10) * time.Minute,
		ConnOpenStrategy: clickhouse.ConnOpenInOrder,
		BlockBufferSize:  10,
	})
	if err != nil {
		logger.Warningf(context.Background(), "CLICKHOUSE_SUBSCRIBER_ALARM", "failed to create clickhouse conn, host %, err: %s", host, err)
		return err
	}
	i.client = conn
	go i.runQueryRecordsTask()
	return nil
}

func (i *ClickHouseSubscriber) Stop() {
	close(i.stopCh)
}

func (i *ClickHouseSubscriber) SubscribeChan() <-chan *protocol.LogGroup {
	return i.channel
}

func (i *ClickHouseSubscriber) FlusherConfig() string {
	return ""
}

func (i *ClickHouseSubscriber) runQueryRecordsTask() {
	for {
		select {
		case <-i.stopCh:
			close(i.channel)
			return
		case <-time.After(time.Duration(i.QueryIntervalMs) * time.Millisecond):
			logGroup, maxTimestamp, err := i.queryRecords()
			if err != nil {
				logger.Warning(context.Background(), "CLICKHOUSE_SUBSCRIBER_ALARM", "err", err)
				continue
			}
			logger.Debugf(context.Background(), "subscriber get new logs maxTimestamp: %d, count: %v", maxTimestamp, len(logGroup.Logs))
			i.channel <- logGroup
			if maxTimestamp > i.lastTimestamp {
				i.lastTimestamp = maxTimestamp
			}
		}
	}
}

func (i *ClickHouseSubscriber) queryRecords() (logGroup *protocol.LogGroup, maxTimestamp int64, err error) {
	logGroup = &protocol.LogGroup{
		Logs: []*protocol.Log{},
	}
	s := fmt.Sprintf(clickhouseQuerySQL, i.Database, i.Table, i.lastTimestamp)
	rows, err := i.client.Query(context.Background(), s)
	if err != nil {
		logger.Warning(context.Background(), "CLICKHOUSE_SUBSCRIBER_ALARM", "err", err)
		return
	}
	logger.Debug(context.Background(), "sql", s)

	defer func() { _ = rows.Close() }()
	for rows.Next() {
		var (
			t int64
			l string
		)
		if err = rows.Scan(&t, &l); err != nil {
			logger.Warning(context.Background(), "CLICKHOUSE_SUBSCRIBER_ALARM", "failed to read clickhouse data, err", err)
			return
		}
		log := &protocol.Log{}
		lc := logContent{}
		if err = json.Unmarshal([]byte(l), &lc); err != nil {
			logger.Warning(context.Background(), "CLICKHOUSE_SUBSCRIBER_ALARM", "failed to unmarshal data, err", err)
			return
		}
		log.Contents = append(log.Contents, &protocol.Log_Content{
			Key:   "_name",
			Value: lc.Name,
		})
		log.Contents = append(log.Contents, &protocol.Log_Content{
			Key:   "_value",
			Value: lc.Value,
		})
		logGroup.Logs = append(logGroup.Logs, log)
	}
	return
}

func (i *ClickHouseSubscriber) buildValue(value interface{}) (typeStr, valueStr string) {
	switch v := value.(type) {
	case string:
		return "string", v
	case float64:
		return "float", strconv.FormatFloat(v, 'g', -1, 64)
	case json.Number:
		return "float", v.String()
	default:
		return "unknown", fmt.Sprintf("%v", value)
	}
}

func init() {
	RegisterCreator(clickHouseName, func(spec map[string]interface{}) (Subscriber, error) {
		i := &ClickHouseSubscriber{
			CreateTable: true,
		}
		if err := mapstructure.Decode(spec, i); err != nil {
			return nil, err
		}

		if i.Addr == "" {
			return nil, errors.New("addr must not be empty")
		}
		if i.Database == "" {
			return nil, errors.New("database must not be empty")
		}
		if i.Table == "" {
			return nil, errors.New("table must no be empty")
		}
		if i.QueryIntervalMs <= 0 {
			i.QueryIntervalMs = 1000
		}
		i.channel = make(chan *protocol.LogGroup, 10)
		i.stopCh = make(chan struct{})
		return i, nil
	})
	doc.Register("subscriber", clickHouseName, new(ClickHouseSubscriber))
}
