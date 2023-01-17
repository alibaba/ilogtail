package profile

import (
	"context"
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
	switch u {
	case NanosecondsUnit, SamplesUnits:
		return "profile_cpu"
	case ObjectsUnit, BytesUnit:
		return "profile_mem"
	case GoroutinesUnits:
		return "profile_goroutines"
	case LockSamplesUnits, LockNanosecondsUnits:
		return "profile_mutex"
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
