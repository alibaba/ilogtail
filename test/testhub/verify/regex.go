package verify

import (
	"fmt"
	"strings"

	"github.com/alibaba/ilogtail/test/testhub/control"
)

const queryRegexSql = "* | SELECT %s FROM log WHERE from_unixtime(__time__) >= from_unixtime(%v) AND from_unixtime(__time__) < now()"

func VerifyRegexSingle(from int32) {
	fields := []string{"mark", "file", "logNo", "ip", "time", "method", "url", "http", "status", "size", "userAgent", "msg"}
	sql := fmt.Sprintf(queryRegexSql, strings.Join(fields, ", "), from)
	if _, err := control.GetLogFromSLS(sql, from); err != nil {
		panic(err)
	}
}
