package verify

import (
	"fmt"
	"strconv"

	"github.com/alibaba/ilogtail/test/testhub/control"
)

const queryCountSql = "* | SELECT COUNT(1) as count FROM log WHERE from_unixtime(__time__) >= from_unixtime(%v) AND from_unixtime(__time__) < now()"

func VerifyLogCount(expect int, from int32) {
	sql := fmt.Sprintf(queryCountSql, from)
	resp, err := control.GetLogFromSLS(sql, from)
	if err != nil {
		panic(err)
	}
	count, err := strconv.Atoi(resp.Body[0]["count"].(string))
	if err != nil {
		panic(err)
	}
	if count != expect {
		panic(fmt.Errorf("log count not match, expect %d, got %d", expect, count))
	}
}
