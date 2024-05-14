package verify

import (
	"fmt"

	"github.com/alibaba/ilogtail/test/testhub/control"
)

const queryCountSql = "SELECT COUNT(*) as count FROM log WHERE from_unixtime(__time__) >= from_unixtime(%v) AND from_unixtime(__time__) < from_unixtime(%v)"

func VerifyLogCount(count int, from, to int32) error {
	sql := fmt.Sprintf(queryCountSql, from, to)
	resp, err := control.GetLogFromSLS(sql, from, to)
	if err != nil {
		return err
	}
	if resp.Body[0]["count"] != count {
		return fmt.Errorf("log count not match, expect %d, got %d", count, resp.Body[0]["count"])
	}
	return nil
}
