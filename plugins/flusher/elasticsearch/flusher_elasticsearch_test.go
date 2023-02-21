package elasticsearch

import (
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/plugins/test"
	"github.com/alibaba/ilogtail/plugins/test/mock"
	"github.com/stretchr/testify/require"
	"strconv"
	"testing"
)

// Invalid Test
func TestFlusherElasticSearch_Flush(t *testing.T) {
	if testing.Short() {
		t.Skip("Skipping integration test in short mode")
	}

	f := NewFlusherElasticSearch()
	f.Addresses = []string{"http://localhost:9200"}
	f.Authentication.PlainText.Username = "elastic"
	f.Authentication.PlainText.Password = "LhJU40CvwGczBA0M*c4P"
	f.Authentication.PlainText.Index = "default"
	f.flusher = f.NormalFlush
	// Verify that we can connect to the ClickHouse
	lctx := mock.NewEmptyContext("p", "l", "c")
	err := f.Init(lctx)
	require.NoError(t, err)

	// Verify that we can successfully write data to the ClickHouse buffer engine table
	lgl := makeTestLogGroupList()
	err = f.Flush("projectName", "logstoreName", "configName", lgl.GetLogGroupList())
	require.NoError(t, err)
}

func makeTestLogGroupList() *protocol.LogGroupList {
	f := map[string]string{}
	lgl := &protocol.LogGroupList{
		LogGroupList: make([]*protocol.LogGroup, 0, 10),
	}
	for i := 1; i <= 10; i++ {
		lg := &protocol.LogGroup{
			Logs: make([]*protocol.Log, 0, 10),
		}
		for j := 1; j <= 10; j++ {
			f["group"] = strconv.Itoa(i)
			f["message"] = "The message: " + strconv.Itoa(j)
			l := test.CreateLogByFields(f)
			lg.Logs = append(lg.Logs, l)
		}
		lgl.LogGroupList = append(lgl.LogGroupList, lg)
	}
	return lgl
}
