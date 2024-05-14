package verify

import (
	"fmt"
	"strings"

	"github.com/alibaba/ilogtail/test/testhub/control"
)

const queryRegexSql = "SELECT %s as count FROM log WHERE from_unixtime(__time__) >= from_unixtime(%v) AND from_unixtime(__time__) < from_unixtime(%v)"

func VerifyRegexSingle(from, to int32) error {
	fields := []string{"mark", "file", "logNo", "time", "level", "msg"}
	sql := fmt.Sprintf(queryRegexSql, strings.Join(fields, ","), from, to)
	_, err := control.GetLogFromSLS(sql, from, to)
	return err
}
