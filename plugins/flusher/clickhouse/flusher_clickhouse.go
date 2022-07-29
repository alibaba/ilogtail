package clickhouse

import (
	"database/sql"
	"encoding/json"
	"fmt"
	"strconv"

	"github.com/ClickHouse/clickhouse-go"
	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
	jsoniter "github.com/json-iterator/go"
)

var insertSQL = "INSERT INTO %s.%s(_time_, _log_) VALUES (?, ?)"

type Flusher struct {
	context       ilogtail.Context
	client        *sql.DB
	Host          string // Clickhouse host
	User          string // Clickhouse user
	Password      string // Clickhouse password
	Database      string // Clickhouse database
	Table         string // Clickhouse table
	BatchSize     int
	WriteTimeout  string
	ReadTimeout   string
	KeyValuePairs bool
}

var (
	DefaultWriteTimeout = "5s"
	DefaultReadTimeout  = "5s"
)

func (f *Flusher) SetUrgent(flag bool) {
}

func (f *Flusher) Stop() error {
	return f.client.Close()
}

func (f *Flusher) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	sql := fmt.Sprintf(insertSQL, f.Database, f.Table)

	// post them to db all at once
	tx, err := f.client.Begin()
	if err != nil {
		return err
	}

	// build statements
	smt, err := tx.Prepare(sql)
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
				writer.WriteObjectField("__time__")
				writer.WriteString(strconv.Itoa(int(log.Time)))
				writer.WriteObjectEnd()
				_, err := smt.Exec(float64(log.Time), string(writer.Buffer()))
				if err != nil {
					return err
				}
			}
		} else {
			logger.Debug(f.context.GetRuntimeContext(), "[LogGroup] topic", logGroup.Topic, "logstore", logGroup.Category, "logcount", len(logGroup.Logs), "tags", logGroup.LogTags)
			for _, log := range logGroup.Logs {
				buf, _ := json.Marshal(log)
				logger.Debug(f.context.GetRuntimeContext(), string(buf))

				_, err := smt.Exec(float64(log.Time), string(buf))
				if err != nil {
					return err
				}
			}
		}
	}

	// commit and record metrics
	if err = tx.Commit(); err != nil {
		return err
	}
	return nil
}

func (f *Flusher) Init(context ilogtail.Context) error {
	f.context = context
	if f.Host == "" {
		var err = fmt.Errorf("clickhouse host is nil")
		logger.Error(f.context.GetRuntimeContext(), "CLICKHOUSE_FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}

	if f.User == "" {
		var err = fmt.Errorf("clickhouse host is nil")
		logger.Error(f.context.GetRuntimeContext(), "CLICKHOUSE_FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}

	if f.Password == "" {
		var err = fmt.Errorf("clickhouse password is nil")
		logger.Error(f.context.GetRuntimeContext(), "CLICKHOUSE_FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}

	if f.Database == "" {
		var err = fmt.Errorf("clickhouse password is nil")
		logger.Error(f.context.GetRuntimeContext(), "CLICKHOUSE_FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}

	if f.Table == "" {
		var err = fmt.Errorf("clickhouse password is nil")
		logger.Error(f.context.GetRuntimeContext(), "CLICKHOUSE_FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}

	writeTimeout := DefaultWriteTimeout
	if f.WriteTimeout != "" {
		writeTimeout = f.WriteTimeout
	}

	readTimeout := DefaultReadTimeout
	if f.ReadTimeout != "" {
		readTimeout = f.ReadTimeout
	}

	dsn := "tcp://" + f.Host + "?username=" + f.User + "&password=" + f.Password + "&database=" + f.Database + "&write_timeout=" + writeTimeout + "&read_timeout=" + readTimeout
	db, err := sql.Open("clickhouse", dsn)
	if err != nil {
		err = fmt.Errorf("sql open failed, err: %w", err)
		logger.Error(f.context.GetRuntimeContext(), "CLICKHOUSE_FLUSHER_INIT_ALARM", "init clickhouse flusher error", err)
		return err
	}

	if err := db.Ping(); err != nil {
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

func (f *Flusher) Description() string {
	return "flush log group with clickhouse"
}

func (f *Flusher) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	return true
}

func init() {
	ilogtail.Flushers["flusher_clickhouse"] = func() ilogtail.Flusher {
		return &Flusher{
			KeyValuePairs: true,
		}
	}
}
