package profile

import (
	"context"
	"time"

	"github.com/gofrs/uuid"
	"github.com/pyroscope-io/pyroscope/pkg/storage/segment"

	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

type Input struct {
	Profile  RawProfile
	Metadata Meta
}

type Stack struct {
	Name  string
	Stack []string
}

type Format string

type CallbackFunc func(id uint64, stack *Stack, vals []uint64, types, units, aggs []string, startTime, endTime int64, labels map[string]string)

const (
	FormatPprof      Format = "pprof"
	FormatJFR        Format = "jfr"
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

func DetectProfileType(valType string) string {
	switch valType {
	case "inuse_space", "inuse_objects", "alloc_space", "alloc_objects", "alloc-size", "alloc-samples", "alloc_in_new_tlab_objects", "alloc_in_new_tlab_bytes", "alloc_outside_tlab_objects", "alloc_outside_tlab_bytes":
		return "profile_mem"
	case "samples", "cpu", "itimer", "lock_count", "lock_duration", "wall":
		return "profile_cpu"
	case "mutex_count", "mutex_duration", "block_duration", "block_count", "contentions", "delay", "lock-time", "lock-count":
		return "profile_mutex"
	case "goroutines", "goroutine":
		return "profile_goroutines"
	case "exception":
		return "profile_exception"
	default:
		return "profile_unknown"
	}
}

func GetProfileID(meta *Meta) string {
	var profileIDStr string
	if meta.Key.HasProfileID() {
		profileIDStr, _ = meta.Key.ProfileID()
	} else {
		profileID, _ := uuid.NewV4()
		profileIDStr = profileID.String()
	}
	return profileIDStr
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
	ParseV2(ctx context.Context, meta *Meta) (groups *models.PipelineGroupEvents, err error)
}
