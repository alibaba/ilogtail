package profile

import (
	"context"
	"strings"
	"time"

	"github.com/pyroscope-io/pyroscope/pkg/storage/segment"

	"github.com/alibaba/ilogtail/pkg/protocol"
)

type Input struct {
	Profile  RawProfile
	Metadata Meta
}

type Format string

const (
	FormatPprof      Format = "pprof"
	FormatJFR        Format = "jfr"
	FormatCollapsed     Format = ""
	FormatTrie       Format = "trie"
	FormatTree       Format = "tree"
	FormatLines      Format = "lines"
	FormatGroups     Format = "groups"
	FormatSpeedscope Format = "speedscope"
)

type Meta struct {
	StartTime       time.Time
	EndTime         time.Time
	Key             *segment.Key
	SpyName         string
	SampleRate      uint32
	Units           Units
	AggregationType AggType
}

type AggType string

const (
	AvgAggType AggType = "avg"
	SumAggType AggType = "sum"
)

type Units string

const (
	SamplesUnits         Units = "samples"
	NanosecondsUnit      Units = "nanoseconds"
	ObjectsUnit          Units = "objects"
	BytesUnit            Units = "bytes"
	GoroutinesUnits      Units = "goroutines"
	LockNanosecondsUnits Units = "lock_nanoseconds"
	LockSamplesUnits     Units = "local_samples"
)

func (u Units) DetectProfileType() string {
	s := string(u)
	switch {
	case strings.Contains(s, string(LockSamplesUnits)), strings.Contains(s, string(LockNanosecondsUnits)):
		return "profile_mutex"
	case strings.Contains(s, string(SamplesUnits)), strings.Contains(s, string(NanosecondsUnit)):
		return "profile_cpu"
	case strings.Contains(s, string(ObjectsUnit)), strings.Contains(s, string(BytesUnit)):
		return "profile_mem"
	case strings.Contains(s, string(GoroutinesUnits)):
		return "profile_goroutines"
	}
	return "profile_unknown"
}

func (u Units) DetectValueType() string {
	switch u {
	case NanosecondsUnit, SamplesUnits:
		return "cpu"
	case ObjectsUnit, BytesUnit:
		return "mem"
	case GoroutinesUnits:
		return "goroutines"
	case LockSamplesUnits, LockNanosecondsUnits:
		return "mutex"
	}
	return "unknown"
}

type RawProfile interface {
	Parse(ctx context.Context, meta *Meta) (logs []*protocol.Log, err error)
}
