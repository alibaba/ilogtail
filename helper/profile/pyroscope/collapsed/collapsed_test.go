package collapsed

import (
	"context"
	"io/ioutil"
	"os"
	"testing"
	"time"

	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/pyroscope-io/pyroscope/pkg/storage/segment"
	"github.com/stretchr/testify/require"
)

func TestParse(t *testing.T) {
	collapsedData, err := readRawFile("./testdata/collapsed.raw")
	if err != nil {
		t.Fatalf("Unable to read collapsed file: %s", err)
	}

	rp := RawProfile{
		RawData: collapsedData,
	}
	logs, err := rp.Parse(context.Background(), &profile.Meta{
		Key:             segment.NewKey(map[string]string{"_app_name_": "12"}),
		SpyName:         "javaspy",
		StartTime:       time.Now(),
		EndTime:         time.Now(),
		SampleRate:      99,
		Units:           profile.SamplesUnits,
		AggregationType: profile.SumAggType,
	})
	require.Equal(t, len(logs), 5)

}

func readRawFile(fname string) ([]byte, error) {
	f, err := os.Open(fname)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	return ioutil.ReadAll(f)
}
