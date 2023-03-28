module github.com/alibaba/ilogtail/pkg

go 1.18

require (
	github.com/alibaba/ilogtail v1.4.0
	github.com/cespare/xxhash v1.1.0
	github.com/cespare/xxhash/v2 v2.2.0
	github.com/cihub/seelog v0.0.0-20170130134532-f561c5e57575
	github.com/gofrs/uuid v4.0.0+incompatible
	github.com/gogo/protobuf v1.3.2
	github.com/golang/snappy v0.0.4
	github.com/influxdata/influxdb v1.11.0
	github.com/influxdata/line-protocol/v2 v2.2.1
	github.com/narqo/go-dogstatsd-parser v0.2.0
	github.com/pierrec/lz4 v2.6.1+incompatible
	github.com/prometheus/common v0.42.0
	github.com/prometheus/prometheus v1.8.2-0.20201119142752-3ad25a6dc3d9
	github.com/pyroscope-io/jfr-parser v0.6.0
	github.com/pyroscope-io/pyroscope v0.0.0-00010101000000-000000000000
	github.com/smartystreets/goconvey v1.7.2
	github.com/stretchr/testify v1.8.2
	go.opentelemetry.io/collector/pdata v0.66.0
	google.golang.org/grpc v1.53.0
	google.golang.org/protobuf v1.29.0
)

require (
	github.com/davecgh/go-spew v1.1.1 // indirect
	github.com/golang/protobuf v1.5.3 // indirect
	github.com/gopherjs/gopherjs v0.0.0-20181017120253-0766667cb4d1 // indirect
	github.com/json-iterator/go v1.1.12 // indirect
	github.com/jtolds/gls v4.20.0+incompatible // indirect
	github.com/matttproud/golang_protobuf_extensions v1.0.4 // indirect
	github.com/modern-go/concurrent v0.0.0-20180306012644-bacd9c7ef1dd // indirect
	github.com/modern-go/reflect2 v1.0.2 // indirect
	github.com/pmezard/go-difflib v1.0.0 // indirect
	github.com/prometheus/client_model v0.3.0 // indirect
	github.com/smartystreets/assertions v1.2.0 // indirect
	github.com/valyala/bytebufferpool v1.0.0 // indirect
	go.uber.org/atomic v1.10.0 // indirect
	go.uber.org/multierr v1.9.0 // indirect
	golang.org/x/net v0.8.0 // indirect
	golang.org/x/sys v0.6.0 // indirect
	golang.org/x/text v0.8.0 // indirect
	google.golang.org/genproto v0.0.0-20230306155012-7f2fa6fef1f4 // indirect
	gopkg.in/yaml.v3 v3.0.1 // indirect
)

replace (
	github.com/VictoriaMetrics/VictoriaMetrics => github.com/iLogtail/VictoriaMetrics v1.83.3-ilogtail
	github.com/VictoriaMetrics/metrics => github.com/iLogtail/metrics v1.23.0-ilogtail
	github.com/aliyun/alibaba-cloud-sdk-go/services/sls_inner => ../external/github.com/aliyun/alibaba-cloud-sdk-go/services/sls_inner
	github.com/elastic/beats/v7 => ../external/github.com/elastic/beats/v7
	github.com/jeromer/syslogparser => ../external/github.com/jeromer/syslogparser
	github.com/mindprince/gonvml => github.com/iLogtail/gonvml v1.0.0
	github.com/pingcap/parser => github.com/iLogtail/parser v0.0.0-20210415081931-48e7f467fd74-ilogtail
	github.com/pyroscope-io/jfr-parser => github.com/iLogtail/jfr-parser v0.6.0
	github.com/pyroscope-io/pyroscope => github.com/iLogtail/pyroscope-lib v0.35.1-ilogtail
	github.com/satori/go.uuid => github.com/satori/go.uuid v1.2.0
	github.com/siddontang/go-mysql => github.com/iLogtail/go-mysql v0.0.0-20180725024449-535abe8f2eba-ilogtail
	github.com/streadway/handy => github.com/iLogtail/handy v0.0.0-20230327021402-6a47ec586270
)
