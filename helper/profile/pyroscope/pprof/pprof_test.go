package pprof

import (
	"context"
	"os"
	"strings"
	"testing"
	"time"

	"github.com/pyroscope-io/pyroscope/pkg/convert/pprof"
	"github.com/pyroscope-io/pyroscope/pkg/storage/segment"
	"github.com/pyroscope-io/pyroscope/pkg/storage/tree"
	"github.com/stretchr/testify/require"

	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/alibaba/ilogtail/pkg/models"
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

func TestRawProfile_ParseV2(t *testing.T) {
	te, err := readPprofFixture("testdata/cpu.pb.gz")
	require.NoError(t, err)
	p := Parser{
		stackFrameFormatter: Formatter{},
		sampleTypesFilter:   filterKnownSamples(DefaultSampleTypeMapping),
	}
	r := new(RawProfile)
	r.group = new(models.PipelineGroupEvents)
	meta := &profile.Meta{
		Key:             segment.NewKey(map[string]string{"_app_name_": "12"}),
		SpyName:         "go",
		StartTime:       time.Now(),
		EndTime:         time.Now(),
		SampleRate:      99,
		Units:           profile.NanosecondsUnit,
		AggregationType: profile.SumAggType,
	}
	cb := r.extraceProfileV2(meta)
	err = extractLogs(context.Background(), te, p, meta, cb)
	require.NoError(t, err)
	group := r.group
	require.Equal(t, 0, group.Group.Metadata.Len())
	require.Equal(t, 0, group.Group.Tags.Len())
	require.Equal(t, 6, len(group.Events))
	event := helper.PickEvent(group.Events, "runtime.kevent /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/sys_darwin.go")
	require.True(t, event != nil)
	m := event.(*models.Profile)

	require.Equal(t, "runtime.kevent /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/sys_darwin.go", m.Name)
	require.Equal(t, models.ProfileStack(strings.Split("runtime.netpoll /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/netpoll_kqueue.go\nruntime.findrunnable /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/proc.go\nruntime.schedule /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/proc.go\nruntime.park_m /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/proc.go\nruntime.mcall /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/asm_arm64.s", "\n")), m.Stack)
	require.Equal(t, "40fb694aa9506d0b", m.StackID)
	require.Equal(t, int64(1619321948265140000), m.StartTime)
	require.Equal(t, int64(1619321949365317167), m.EndTime)
	require.Equal(t, "go", m.Language)
	require.Equal(t, "profile_cpu", m.ProfileType.String())
	require.Equal(t, "CallStack", m.DataType)
	require.Equal(t, models.NewTagsWithMap(map[string]string{
		"_app_name_": "12",
	}), m.Tags)
	require.Equal(t, models.ProfileValues{
		models.NewProfileValue("cpu", "nanoseconds", "sum", 250000000),
	}, m.Values)
}

func TestRawProfile_Parse(t *testing.T) {
	te, err := readPprofFixture("testdata/cpu.pb.gz")
	require.NoError(t, err)
	p := Parser{
		stackFrameFormatter: Formatter{},
		sampleTypesFilter:   filterKnownSamples(DefaultSampleTypeMapping),
	}
	r := new(RawProfile)
	meta := &profile.Meta{
		Key:             segment.NewKey(map[string]string{"_app_name_": "12"}),
		SpyName:         "go",
		StartTime:       time.Now(),
		EndTime:         time.Now(),
		SampleRate:      99,
		Units:           profile.NanosecondsUnit,
		AggregationType: profile.SumAggType,
	}
	cb := r.extraceProfileV1(meta)
	err = extractLogs(context.Background(), te, p, meta, cb)
	require.NoError(t, err)
	logs := r.logs
	require.Equal(t, len(logs), 6)
	picks := helper.PickLogs(logs, "stackID", "40fb694aa9506d0b")
	require.Equal(t, len(picks), 1)
	log := picks[0]
	require.Equal(t, helper.ReadLogVal(log, "name"), "runtime.kevent /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/sys_darwin.go")
	require.Equal(t, helper.ReadLogVal(log, "stack"), "runtime.netpoll /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/netpoll_kqueue.go\nruntime.findrunnable /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/proc.go\nruntime.schedule /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/proc.go\nruntime.park_m /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/proc.go\nruntime.mcall /opt/homebrew/Cellar/go/1.16.1/libexec/src/runtime/asm_arm64.s")
	require.Equal(t, helper.ReadLogVal(log, "language"), "go")
	require.Equal(t, helper.ReadLogVal(log, "type"), "profile_cpu")
	require.Equal(t, helper.ReadLogVal(log, "units"), "nanoseconds")
	require.Equal(t, helper.ReadLogVal(log, "valueTypes"), "cpu")
	require.Equal(t, helper.ReadLogVal(log, "aggTypes"), "sum")
	require.Equal(t, helper.ReadLogVal(log, "dataType"), "CallStack")
	require.Equal(t, helper.ReadLogVal(log, "durationNs"), "1100177167")
	require.Equal(t, helper.ReadLogVal(log, "labels"), "{\"_app_name_\":\"12\"}")
	require.Equal(t, helper.ReadLogVal(log, "value_0"), "250000000")
}
