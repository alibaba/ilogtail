package pprof

import (
	"context"
	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/pyroscope-io/pyroscope/pkg/convert/pprof"
	"github.com/pyroscope-io/pyroscope/pkg/storage/segment"
	"github.com/pyroscope-io/pyroscope/pkg/storage/tree"
	"github.com/stretchr/testify/require"
	"os"
	"testing"
	"time"
)

func readPprofFixture(path string) (*tree.Profile, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer func() {
		_ = f.Close()
	}()

	var p tree.Profile
	if err = pprof.Decode(f, &p); err != nil {
		return nil, err
	}
	return &p, nil
}

func TestRawProfile_Parse(t *testing.T) {
	te, err := readPprofFixture("testdata/cpu.pb.gz")
	require.NoError(t, err)
	p := Parser{
		stackFrameFormatter: Formatter{},
		sampleTypesFilter:   filterKnownSamples(DefaultSampleTypeMapping),
	}
	var logs []*protocol.Log
	logs, err = extractLogs(context.Background(), te, p, &profile.Meta{
		Key:             segment.NewKey(map[string]string{"_app_name_": "12"}),
		SpyName:         "gospy",
		StartTime:       time.Now(),
		EndTime:         time.Now(),
		SampleRate:      99,
		Units:           profile.NanosecondsUnit,
		AggregationType: profile.SumAggType,
	}, logs)
	require.NoError(t, err)
	require.Equal(t, len(logs), 6)
	picks := helper.PickLogs(logs, "stackID", "5765359752373105471")
	require.Equal(t, len(picks), 1)
	log := picks[0]
	require.Equal(t, helper.ReadLogVal(log, "name"), "runtime.kevent /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/sys_darwin.go")
	require.Equal(t, helper.ReadLogVal(log, "stack"), "runtime.kevent /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/sys_darwin.go\nruntime.netpoll /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/netpoll_kqueue.go\nruntime.findrunnable /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/proc.go\nruntime.schedule /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/proc.go\nruntime.park_m /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/proc.go\nruntime.mcall /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/asm_arm64.s")
	require.Equal(t, helper.ReadLogVal(log, "__tag__:language"), "go")
	require.Equal(t, helper.ReadLogVal(log, "__tag__:type"), "profile_cpu")
	require.Equal(t, helper.ReadLogVal(log, "__tag__:units"), "nanoseconds")
	require.Equal(t, helper.ReadLogVal(log, "__tag__:valueTypes"), "cpu")
	require.Equal(t, helper.ReadLogVal(log, "__tag__:aggTypes"), "sum")
	require.Equal(t, helper.ReadLogVal(log, "__tag__:dataType"), "CallStack")
	require.Equal(t, helper.ReadLogVal(log, "__tag__:durationNs"), "1100177167")
	require.Equal(t, helper.ReadLogVal(log, "__tag__:labels"), "{\"_sample_rate_\":\"99\"}")
	require.Equal(t, helper.ReadLogVal(log, "value_0"), "250000000")
}
