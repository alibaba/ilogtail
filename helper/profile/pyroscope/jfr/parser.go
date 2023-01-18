package jfr

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"regexp"
	"strconv"
	"strings"

	"github.com/cespare/xxhash"
	"github.com/gofrs/uuid"
	"github.com/pyroscope-io/jfr-parser/parser"
	"github.com/pyroscope-io/pyroscope/pkg/storage/metadata"
	"github.com/pyroscope-io/pyroscope/pkg/storage/segment"
	"github.com/pyroscope-io/pyroscope/pkg/storage/tree"

	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

const (
	_ = iota
	sampleTypeCPU
	sampleTypeWall
	sampleTypeInTLABObjects
	sampleTypeInTLABBytes
	sampleTypeOutTLABObjects
	sampleTypeOutTLABBytes
	sampleTypeLockSamples
	sampleTypeLockDuration
)

func ParseJFR(ctx context.Context, meta *profile.Meta, body io.Reader, jfrLabels *LabelsSnapshot) (logs []*protocol.Log, err error) {
	chunks, err := parser.ParseWithOptions(body, &parser.ChunkParseOptions{
		CPoolProcessor: processSymbols,
	})
	if err != nil {
		return nil, fmt.Errorf("unable to parse JFR format: %w", err)
	}
	for _, c := range chunks {
		oneChunkLogs, pErr := parse(ctx, meta, c, jfrLabels)
		if pErr != nil {
			//err = multierror.Append(err, pErr)
			return nil, err
		}
		logs = append(logs, oneChunkLogs...)
	}
	return logs, nil
}

// revive:disable-next-line:cognitive-complexity necessary complexity
func parse(ctx context.Context, meta *profile.Meta, c parser.Chunk, jfrLabels *LabelsSnapshot) (logs []*protocol.Log, err error) {
	stackMap := make(map[uint64]string)
	valMap := make(map[uint64][]uint64)
	labelMap := make(map[uint64]map[string]string)
	typeMap := make(map[uint64][]string)
	unitMap := make(map[uint64][]string)
	aggtypeMap := make(map[uint64][]string)

	var event string
	for _, e := range c.Events {
		if as, ok := e.(*parser.ActiveSetting); ok {
			if as.Name == "event" {
				event = as.Value
			}
		}
	}
	cache := make(tree.LabelsCache)
	for contextID, events := range groupEventsByContextID(c.Events) {
		labels := getContextLabels(contextID, jfrLabels)
		lh := labels.Hash()
		for _, e := range events {
			switch e.(type) {
			case *parser.ExecutionSample:
				es := e.(*parser.ExecutionSample)
				if fs := frames(es.StackTrace); fs != nil {
					if es.State.Name == "STATE_RUNNABLE" {
						cache.GetOrCreateTreeByHash(sampleTypeCPU, labels, lh).InsertStackString(fs, 1)
					}
					cache.GetOrCreateTreeByHash(sampleTypeWall, labels, lh).InsertStackString(fs, 1)
				}
			case *parser.ObjectAllocationInNewTLAB:
				oa := e.(*parser.ObjectAllocationInNewTLAB)
				if fs := frames(oa.StackTrace); fs != nil {
					cache.GetOrCreateTreeByHash(sampleTypeInTLABObjects, labels, lh).InsertStackString(fs, 1)
					cache.GetOrCreateTreeByHash(sampleTypeInTLABBytes, labels, lh).InsertStackString(fs, uint64(oa.TLABSize))
				}
			case *parser.ObjectAllocationOutsideTLAB:
				oa := e.(*parser.ObjectAllocationOutsideTLAB)
				if fs := frames(oa.StackTrace); fs != nil {
					cache.GetOrCreateTreeByHash(sampleTypeOutTLABObjects, labels, lh).InsertStackString(fs, 1)
					cache.GetOrCreateTreeByHash(sampleTypeOutTLABBytes, labels, lh).InsertStackString(fs, uint64(oa.AllocationSize))
				}
			case *parser.JavaMonitorEnter:
				jme := e.(*parser.JavaMonitorEnter)
				if fs := frames(jme.StackTrace); fs != nil {
					cache.GetOrCreateTreeByHash(sampleTypeLockSamples, labels, lh).InsertStackString(fs, 1)
					cache.GetOrCreateTreeByHash(sampleTypeLockDuration, labels, lh).InsertStackString(fs, uint64(jme.Duration))
				}
			case *parser.ThreadPark:
				tp := e.(*parser.ThreadPark)
				if fs := frames(tp.StackTrace); fs != nil {
					cache.GetOrCreateTreeByHash(sampleTypeLockSamples, labels, lh).InsertStackString(fs, 1)
					cache.GetOrCreateTreeByHash(sampleTypeLockDuration, labels, lh).InsertStackString(fs, uint64(tp.Duration))
				}
			}
		}
	}
	for sampleType, entries := range cache {
		for _, e := range entries {
			if i := labelIndex(jfrLabels, e.Labels, segment.ProfileIDLabelName); i != -1 {
				cutLabels := tree.CutLabel(e.Labels, i)
				cache.GetOrCreateTree(sampleType, cutLabels).Merge(e.Tree)
			}
		}
	}
	cb := func(n string, labels tree.Labels, t *tree.Tree, u metadata.Units) {
		key := buildKey(n, meta.Key.Labels(), labels, jfrLabels)

		labelsMap := make(map[string]string)
		for lk, lv := range key.Labels() {
			labelsMap[lk] = lv
		}

		if name := key.AppName(); name != "" {
			labelsMap["_app_name_"] = name
		}
		if meta.SampleRate > 0 {
			labelsMap["_sample_rate_"] = strconv.FormatUint(uint64(meta.SampleRate), 10)
		}

		t.IterateStacks(func(name string, self uint64, stack []string) {
			fs := name + splitor + strings.Join(stack[1:], "\n")
			id := xxhash.Sum64String(fs)
			stackMap[id] = fs
			aggtypeMap[id] = append(aggtypeMap[id], string(meta.AggregationType))
			typeMap[id] = append(typeMap[id], n)
			unitMap[id] = append(unitMap[id], u.String())
			valMap[id] = append(valMap[id], self)
			labelMap[id] = labelsMap
		})
	}
	for sampleType, entries := range cache {
		if sampleType == sampleTypeWall && event != "wall" {
			continue
		}
		n := getName(sampleType, event)
		units := getUnits(sampleType)
		for _, e := range entries {
			cb(n, e.Labels, e.Tree, units)
		}
	}

	var profileIDStr string
	if meta.Key.HasProfileID() {
		profileIDStr, _ = meta.Key.ProfileID()
	} else {
		profileID, _ := uuid.NewV4()
		profileIDStr = profileID.String()
	}

	meta.SpyName = strings.TrimSuffix(meta.SpyName, "spy")
	for id, fs := range stackMap {
		if len(valMap[id]) == 0 || len(typeMap[id]) == 0 || len(unitMap[id]) == 0 || len(labelMap[id]) == 0 {
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
				Key:   "language",
				Value: meta.SpyName,
			},
			&protocol.Log_Content{
				Key:   "type",
				Value: profile.Units(unitMap[id][0]).DetectProfileType(),
			},
			&protocol.Log_Content{
				Key:   "units",
				Value: strings.Join(unitMap[id], ","),
			},
			&protocol.Log_Content{
				Key:   "valueTypes",
				Value: strings.Join(typeMap[id], ","),
			},
			&protocol.Log_Content{
				Key:   "aggTypes",
				Value: strings.Join(aggtypeMap[id], ","),
			},
			&protocol.Log_Content{
				Key:   "dataType",
				Value: "CallStack",
			},
			&protocol.Log_Content{
				Key:   "durationNs",
				Value: strconv.FormatInt(meta.EndTime.Sub(meta.StartTime).Nanoseconds(), 10),
			},

			&protocol.Log_Content{
				Key:   "profileID",
				Value: profileIDStr,
			},
			&protocol.Log_Content{
				Key:   "labels",
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

	return logs, err
}

func getName(sampleType int64, event string) string {
	switch sampleType {
	case sampleTypeCPU:
		if event == "cpu" || event == "itimer" || event == "wall" {
			profile := event
			if event == "wall" {
				profile = "cpu"
			}
			return profile
		}
	case sampleTypeWall:
		return "wall"
	case sampleTypeInTLABObjects:
		return "alloc_in_new_tlab_objects"
	case sampleTypeInTLABBytes:
		return "alloc_in_new_tlab_bytes"
	case sampleTypeOutTLABObjects:
		return "alloc_outside_tlab_objects"
	case sampleTypeOutTLABBytes:
		return "alloc_outside_tlab_bytes"
	case sampleTypeLockSamples:
		return "lock_count"
	case sampleTypeLockDuration:
		return "lock_duration"
	}
	return "unknown"
}

func getUnits(sampleType int64) metadata.Units {
	switch sampleType {
	case sampleTypeCPU:
		return metadata.SamplesUnits
	case sampleTypeWall:
		return metadata.SamplesUnits
	case sampleTypeInTLABObjects:
		return metadata.ObjectsUnits
	case sampleTypeInTLABBytes:
		return metadata.BytesUnits
	case sampleTypeOutTLABObjects:
		return metadata.ObjectsUnits
	case sampleTypeOutTLABBytes:
		return metadata.BytesUnits
	case sampleTypeLockSamples:
		return metadata.LockSamplesUnits
	case sampleTypeLockDuration:
		return metadata.LockNanosecondsUnits
	}
	return metadata.SamplesUnits
}

func buildKey(n string, appLabels map[string]string, labels tree.Labels, snapshot *LabelsSnapshot) *segment.Key {
	finalLabels := map[string]string{}
	for k, v := range appLabels {
		finalLabels[k] = v
	}
	for _, v := range labels {
		ks, ok := snapshot.Strings[v.Key]
		if !ok {
			continue
		}
		vs, ok := snapshot.Strings[v.Str]
		finalLabels[ks] = vs
	}
	// finalLabels["__name__"] += "." + n
	return segment.NewKey(finalLabels)
}

func getContextLabels(contextID int64, labels *LabelsSnapshot) tree.Labels {
	if contextID == 0 {
		return nil
	}
	var ctx *Context
	var ok bool
	if ctx, ok = labels.Contexts[contextID]; !ok {
		return nil
	}
	res := make(tree.Labels, 0, len(ctx.Labels))
	for k, v := range ctx.Labels {
		res = append(res, &tree.Label{Key: k, Str: v})
	}
	return res
}
func labelIndex(s *LabelsSnapshot, labels tree.Labels, key string) int {
	for i, label := range labels {
		if n, ok := s.Strings[label.Key]; ok {
			if n == key {
				return i
			}
		}
	}
	return -1
}

func groupEventsByContextID(events []parser.Parseable) map[int64][]parser.Parseable {
	res := make(map[int64][]parser.Parseable)
	for _, e := range events {
		switch e.(type) {
		case *parser.ExecutionSample:
			es := e.(*parser.ExecutionSample)
			res[es.ContextId] = append(res[es.ContextId], e)
		case *parser.ObjectAllocationInNewTLAB:
			oa := e.(*parser.ObjectAllocationInNewTLAB)
			res[oa.ContextId] = append(res[oa.ContextId], e)
		case *parser.ObjectAllocationOutsideTLAB:
			oa := e.(*parser.ObjectAllocationOutsideTLAB)
			res[oa.ContextId] = append(res[oa.ContextId], e)
		case *parser.JavaMonitorEnter:
			jme := e.(*parser.JavaMonitorEnter)
			res[jme.ContextId] = append(res[jme.ContextId], e)
		case *parser.ThreadPark:
			tp := e.(*parser.ThreadPark)
			res[tp.ContextId] = append(res[tp.ContextId], e)
		}
	}
	return res
}

func frames(st *parser.StackTrace) []string {
	if st == nil {
		return nil
	}
	frames := make([]string, 0, len(st.Frames))
	for i := len(st.Frames) - 1; i >= 0; i-- {
		f := st.Frames[i]
		// TODO(abeaumont): Add support for line numbers.
		if f.Method != nil && f.Method.Type != nil && f.Method.Type.Name != nil && f.Method.Name != nil {
			frames = append(frames, f.Method.Type.Name.String+"."+f.Method.Name.String)
		}
	}
	return frames
}

// jdk/internal/reflect/GeneratedMethodAccessor31
var generatedMethodAccessor = regexp.MustCompile("^(jdk/internal/reflect/GeneratedMethodAccessor)(\\d+)$")

// org/example/rideshare/OrderService$$Lambda$669.0x0000000800fd7318.run
var lambdaGeneratedEnclosingClass = regexp.MustCompile("^(.+\\$\\$Lambda\\$)\\d+[./](0x[\\da-f]+|\\d+)$")

// libzstd-jni-1.5.1-16931311898282279136.so.Java_com_github_luben_zstd_ZstdInputStreamNoFinalizer_decompressStream
var zstdJniSoLibName = regexp.MustCompile("^(\\.?/tmp/)?(libzstd-jni-\\d+\\.\\d+\\.\\d+-)(\\d+)(\\.so)( \\(deleted\\))?$")

// ./tmp/libamazonCorrettoCryptoProvider109b39cf33c563eb.so
var amazonCorrettoCryptoProvider = regexp.MustCompile("^(\\.?/tmp/)?(libamazonCorrettoCryptoProvider)([0-9a-f]{16})(\\.so)( \\(deleted\\))?$")

// libasyncProfiler-linux-arm64-17b9a1d8156277a98ccc871afa9a8f69215f92.so
var pyroscopeAsyncProfiler = regexp.MustCompile(
	"^(\\.?/tmp/)?(libasyncProfiler)-(linux-arm64|linux-musl-x64|linux-x64|macos)-(17b9a1d8156277a98ccc871afa9a8f69215f92)(\\.so)( \\(deleted\\))?$")

func mergeJVMGeneratedClasses(frame string) string {
	frame = generatedMethodAccessor.ReplaceAllString(frame, "${1}_")
	frame = lambdaGeneratedEnclosingClass.ReplaceAllString(frame, "${1}_")
	frame = zstdJniSoLibName.ReplaceAllString(frame, "libzstd-jni-_.so")
	frame = amazonCorrettoCryptoProvider.ReplaceAllString(frame, "libamazonCorrettoCryptoProvider_.so")
	frame = pyroscopeAsyncProfiler.ReplaceAllString(frame, "libasyncProfiler-_.so")
	return frame
}

func processSymbols(meta parser.ClassMetadata, cpool *parser.CPool) {
	if meta.Name == "jdk.types.Symbol" {
		for _, v := range cpool.Pool {
			sym := v.(*parser.Symbol)
			sym.String = mergeJVMGeneratedClasses(sym.String)
		}
	}
}
