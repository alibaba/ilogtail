package pprof

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"mime/multipart"
	"strconv"
	"strings"

	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"

	"github.com/cespare/xxhash/v2"
	"github.com/gofrs/uuid"
	"github.com/pyroscope-io/pyroscope/pkg/convert/pprof"
	"github.com/pyroscope-io/pyroscope/pkg/storage/metadata"
	"github.com/pyroscope-io/pyroscope/pkg/storage/tree"
	"github.com/pyroscope-io/pyroscope/pkg/util/form"
)

const (
	formFieldProfile, formFieldSampleTypeConfig = "profile", "sample_type_config"
	splitor                                     = "$@$"
)

var DefaultSampleTypeMapping = map[string]*tree.SampleTypeConfig{
	"cpu": {
		Units:   metadata.Units(profile.NanosecondsUnit),
		Sampled: true,
	},
	"inuse_objects": {
		Units:       metadata.ObjectsUnits,
		Aggregation: "avg",
	},
	"alloc_objects": {
		Units:      metadata.ObjectsUnits,
		Cumulative: true,
	},
	"inuse_space": {
		Units:       metadata.BytesUnits,
		Aggregation: "avg",
	},
	"alloc_space": {
		Units:      metadata.BytesUnits,
		Cumulative: true,
	},
	"goroutine": {
		DisplayName: "goroutines",
		Units:       metadata.GoroutinesUnits,
		Aggregation: "avg",
	},
	"contentions": {
		DisplayName: "mutex_count",
		Units:       metadata.LockSamplesUnits,
		Cumulative:  true,
	},
	"delay": {
		DisplayName: "mutex_duration",
		Units:       metadata.LockNanosecondsUnits,
		Cumulative:  true,
	},
}

type RawProfile struct {
	RawData             []byte
	FormDataContentType string

	profile          []byte
	sampleTypeConfig map[string]*tree.SampleTypeConfig
}

func (r *RawProfile) Parse(ctx context.Context, meta *profile.Meta) (logs []*protocol.Log, err error) {
	if err = r.extractProfile(); err != nil {
		return nil, fmt.Errorf("cannot extract profile: %w", err)
	}
	if len(r.profile) == 0 {
		return nil, errors.New("empty profile")
	}
	err = pprof.DecodePool(bytes.NewReader(r.profile), func(tf *tree.Profile) error {
		if r.sampleTypeConfig == nil {
			r.sampleTypeConfig = DefaultSampleTypeMapping
		}
		p := Parser{
			stackFrameFormatter: Formatter{},
			sampleTypesFilter:   filterKnownSamples(r.sampleTypeConfig),
			sampleTypes:         r.sampleTypeConfig,
		}
		if logs, err = extractLogs(ctx, tf, p, meta, logs); err != nil {
			return err
		}
		return nil
	})
	return logs, err
}

func extractLogs(ctx context.Context, tp *tree.Profile, p Parser, meta *profile.Meta, logs []*protocol.Log) ([]*protocol.Log, error) {
	stackMap := make(map[uint64]string)
	valMap := make(map[uint64][]uint64)
	labelMap := make(map[uint64]map[string]string)
	typeMap := make(map[uint64][]string)
	unitMap := make(map[uint64][]string)
	aggtypeMap := make(map[uint64][]string)

	var profileIDStr string
	if meta.Key.HasProfileID() {
		profileIDStr, _ = meta.Key.ProfileID()
	} else {
		profileID, _ := uuid.NewV4()
		profileIDStr = profileID.String()
	}

	err := p.iterate(tp, func(vt *tree.ValueType, tl tree.Labels, t *tree.Tree) (keep bool, err error) {
		if len(tp.StringTable) <= int(vt.Type) || len(tp.StringTable) <= int(vt.Unit) {
			return true, errors.New("invalid type or unit")
		}
		stype := tp.StringTable[vt.Type]
		sunit := tp.StringTable[vt.Unit]

		labelsMap := make(map[string]string)
		for _, l := range tl {
			if len(tp.StringTable) <= int(l.GetKey()) || len(tp.StringTable) <= int(l.GetStr()) {
				if logger.DebugFlag() {
					logger.Debug(ctx, "found illegal labels index, would skip it", vt.String())
				}
				continue
			}
			k := tp.StringTable[l.GetKey()]
			v := tp.StringTable[l.GetStr()]
			labelsMap[k] = v
		}
		if name := meta.Key.AppName(); name != "" {
			labelsMap["_app_name_"] = name
		}
		if meta.SampleRate > 0 {
			labelsMap["_sample_rate_"] = strconv.FormatUint(uint64(meta.SampleRate), 10)
		}
		for k, v := range meta.Key.Labels() {
			labelsMap[k] = v
		}

		t.IterateStacks(func(name string, self uint64, stack []string) {
			fs := name + splitor + strings.Join(stack, "\n")
			id := xxhash.Sum64String(fs)
			stackMap[id] = fs
			aggtypeMap[id] = append(aggtypeMap[id], p.getAggregationType(stype, string(meta.AggregationType)))
			typeMap[id] = append(typeMap[id], p.getDisplayName(stype))
			unitMap[id] = append(unitMap[id], sunit)
			valMap[id] = append(valMap[id], self)
			labelMap[id] = labelsMap
		})
		return true, nil
	})
	if err != nil {
		return nil, fmt.Errorf("iterate profile tree error: %w", err)
	}
	meta.SpyName = strings.TrimSuffix(meta.SpyName, "spy")
	for id, fs := range stackMap {
		if len(valMap[id]) == 0 || len(typeMap[id]) == 0 || len(unitMap[id]) == 0 || labelMap[id] == nil {
			logger.Warning(ctx, "PPROF_PROFILE_ALARM", "stack don't have enough meta or values", fs)
			continue
		}
		var content []*protocol.Log_Content
		idx := strings.Index(fs, splitor)
		b, _ := json.Marshal(labelMap[id])

		content = append(content,
			&protocol.Log_Content{
				Key:   "name",
				Value: fs[:idx],
			},
			&protocol.Log_Content{
				Key:   "stack",
				Value: fs[idx+3:],
			},
			&protocol.Log_Content{
				Key:   "stackID",
				Value: strconv.FormatUint(id, 10),
			},
			&protocol.Log_Content{
				Key:   "__tag__:language",
				Value: meta.SpyName,
			},
			&protocol.Log_Content{
				Key:   "__tag__:type",
				Value: meta.Units.DetectProfileType(),
			},
			&protocol.Log_Content{
				Key:   "__tag__:units",
				Value: strings.Join(unitMap[id], ","),
			},
			&protocol.Log_Content{
				Key:   "__tag__:valueTypes",
				Value: strings.Join(typeMap[id], ","),
			},
			&protocol.Log_Content{
				Key:   "__tag__:aggTypes",
				Value: strings.Join(aggtypeMap[id], ","),
			},
			&protocol.Log_Content{
				Key:   "__tag__:dataType",
				Value: "CallStack",
			},
			&protocol.Log_Content{
				Key:   "__tag__:durationNs",
				Value: strconv.FormatInt(tp.GetDurationNanos(), 10),
			},
			&protocol.Log_Content{
				Key:   "__tag__:profileID",
				Value: profileIDStr,
			},
			&protocol.Log_Content{
				Key:   "__tag__:labels",
				Value: string(b),
			},
		)
		for i, v := range valMap[id] {
			content = append(content, &protocol.Log_Content{
				Key:   fmt.Sprintf("value_%d", i),
				Value: strconv.FormatUint(v, 10),
			})
		}
		log := &protocol.Log{
			Time:     uint32(meta.StartTime.Unix()),
			Contents: content,
		}
		logs = append(logs, log)
	}
	return logs, nil
}

func (r *RawProfile) extractProfile() error {
	if r.FormDataContentType == "" {
		r.profile = r.RawData
		return nil
	}
	boundary, err := form.ParseBoundary(r.FormDataContentType)
	if err != nil {
		return err
	}
	f, err := multipart.NewReader(bytes.NewReader(r.RawData), boundary).ReadForm(32 << 20)
	if err != nil {
		return err
	}
	defer func() {
		_ = f.RemoveAll()
	}()

	if r.profile, err = form.ReadField(f, formFieldProfile); err != nil {
		return err
	}
	if c, err := form.ReadField(f, formFieldSampleTypeConfig); err != nil {
		return err
	} else if c != nil {
		var config map[string]*tree.SampleTypeConfig
		if err = json.Unmarshal(c, &config); err != nil {
			return err
		}
		r.sampleTypeConfig = config
	}
	return nil
}
