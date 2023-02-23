package elasticsearch

import (
	"fmt"
	"github.com/alibaba/ilogtail/pkg/tlscommon"
	"github.com/elastic/go-elasticsearch/v8"
	"io/ioutil"
	"net/http"
	"time"
)

type Authentication struct {
	//PlainTextConfig
	PlainText *PlainTextConfig
	// TLS authentication
	TLS *tlscommon.TLSConfig
	//HTTP config
	HttpConfig *HttpConfig
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
	ResponseHeaderTimeout string
}

func (config *Authentication) ConfigureAuthentication(opts *elasticsearch.Config) error {
	if config.PlainText != nil {
		if err := config.PlainText.ConfigurePlaintext(opts); err != nil {
			return err
		}
	}
	if config.TLS != nil || config.HttpConfig != nil {
		if err := configureTLSandHttp(config.HttpConfig, config.TLS, opts); err != nil {
			return err
		}
	}
	return nil
}

func (plainTextConfig *PlainTextConfig) ConfigurePlaintext(opts *elasticsearch.Config) error {
	// Validate Auth info
	if plainTextConfig.Username != "" && plainTextConfig.Password == "" {
		return fmt.Errorf("PlainTextConfig password must be set when username is configured")
	}
	opts.Username = plainTextConfig.Username
	opts.Password = plainTextConfig.Password
	return nil
}

func configureTLSandHttp(httpcfg *HttpConfig, config *tlscommon.TLSConfig, opts *elasticsearch.Config) error {
	tlsConfig, err := config.LoadTLSConfig()
	if err != nil {
		return fmt.Errorf("error loading tls config: %w", err)
	}
	transport := http.Transport{}
	if tlsConfig != nil && tlsConfig.InsecureSkipVerify {
		transport.TLSClientConfig = tlsConfig
	}
	if httpcfg.MaxIdleConnsPerHost != 0 {
		transport.MaxIdleConnsPerHost = httpcfg.MaxIdleConnsPerHost
	}
	if httpcfg.ResponseHeaderTimeout != "" {
		unit, err := convertTimeUnit(httpcfg.ResponseHeaderTimeout)
		if err == nil {
			transport.ResponseHeaderTimeout = unit
		}
	}
	opts.Transport = &transport
	if config.CAFile != "" {
		opts.CACert, err = loadCert(config.CAFile)
	}
	return nil
}

func loadCert(filePath string) ([]byte, error) {
	return ioutil.ReadFile(filePath)
}

func convertTimeUnit(unit string) (time.Duration, error) {
	val, ok := timeUnitMap[unit]
	if !ok {
		return 0, fmt.Errorf("unsupported time unit: %q", unit)
	}
	return val, nil

}

var timeUnitMap = map[string]time.Duration{
	"Nanosecond":  time.Nanosecond,
	"Microsecond": time.Microsecond,
	"Millisecond": time.Millisecond,
	"Second":      time.Second,
	"Minute":      time.Minute,
	"Hour":        time.Hour,
}
