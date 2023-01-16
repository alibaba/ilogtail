package jfr

import (
	"bytes"
	"compress/gzip"
	"context"
	"io"
	"io/ioutil"
	"os"
	"testing"
	"time"

	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/pyroscope-io/pyroscope/pkg/storage/segment"
	"github.com/stretchr/testify/require"
	"google.golang.org/protobuf/proto"
)

func TestRawProfile_Parse(t *testing.T) {

}

func TestParse(t *testing.T) {
	jfr, err := readGzipFile("./testdata/example.jfr.gz")
	if err != nil {
		t.Fatalf("Unable to read JFR file: %s", err)
	}
	/*
		expectedJson, err := readGzipFile("./testdata/example_parsed.json.gz")
		if err != nil {
			t.Fatalf("Unable to read example_parsd.json")
		}
	*/

	rp := RawProfile{
		FormDataContentType: "",
		RawData:             jfr,
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
	if err != nil {
		t.Fatalf("Failed to parse JFR: %s", err)
		return
	}
	require.Equal(t, len(logs), 3)
}

func TestParseJFR(t *testing.T) {
	jfr, err := readRawFile("./testdata/jfr.raw")
	if err != nil {
		t.Fatalf("Unable to read JFR file: %s", err)
	}

	jfrLables, err := readRawFile("./testdata/jfr_labels.raw")
	if err != nil {
		t.Fatalf("Unable to read JFR label file: %s", err)
	}
	var labels LabelsSnapshot

	if err = proto.Unmarshal(jfrLables, &labels); err != nil {
		t.Fatalf("Unable to read JFR label file: %s", err)
	}

	var reader io.Reader = bytes.NewReader(jfr)

	logs, err := ParseJFR(context.Background(), &profile.Meta{
		Key:             segment.NewKey(map[string]string{"_app_name_": "12"}),
		SpyName:         "javaspy",
		StartTime:       time.Now(),
		EndTime:         time.Now(),
		SampleRate:      99,
		Units:           profile.SamplesUnits,
		AggregationType: profile.SumAggType,
	}, reader, &labels)
	require.Equal(t, len(logs), 3)
}

func readGzipFile(fname string) ([]byte, error) {
	f, err := os.Open(fname)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	r, err := gzip.NewReader(f)
	if err != nil {
		return nil, err
	}
	defer r.Close()
	return ioutil.ReadAll(r)
}

func readRawFile(fname string) ([]byte, error) {
	f, err := os.Open(fname)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	return ioutil.ReadAll(f)
}
