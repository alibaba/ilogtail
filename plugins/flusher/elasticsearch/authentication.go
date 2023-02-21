package elasticsearch

import (
	"github.com/alibaba/ilogtail/pkg/tlscommon"
	"time"
)

type Authentication struct {
	//PlainTextConfig
	PlainText *PlainTextConfig
	// TLS authentication
	TLS *tlscommon.TLSConfig
	//HTTP config
	HttpConfig HttpConfig
}

type PlainTextConfig struct {
	// The username for connecting to clickhouse.
	Username string
	// The password for connecting to clickhouse.
	Password string
	// The container of logs
	Index string
}

type HttpConfig struct {
	MaxIdleConnsPerHost   int
	ResponseHeaderTimeout time.Duration
}
