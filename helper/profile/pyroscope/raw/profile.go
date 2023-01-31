package raw

import (
	"bufio"
	"bytes"
	"context"
	"encoding/json"
	"strconv"
	"strings"

	"github.com/cespare/xxhash/v2"
	"github.com/pyroscope-io/pyroscope/pkg/structs/transporttrie"

	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

type Profile struct {
	RawData []byte
	Format  profile.Format

	// v1 result
	logs []*protocol.Log
	// v2 result
	group models.PipelineGroupEvents
}

func (p *Profile) ParseV2(ctx context.Context, meta *profile.Meta) (group *models.PipelineGroupEvents, err error) {
	cb := p.extractProfileV2(meta)
	if err := p.doParse(cb); err != nil {
		return nil, err
	}
	return &p.group, nil
}

func (p *Profile) Parse(ctx context.Context, meta *profile.Meta) (logs []*protocol.Log, err error) {
	cb := p.extractProfileV1(meta)
	if err := p.doParse(cb); err != nil {
		return nil, err
	}
	return p.logs, nil
}

func (p *Profile) doParse(cb func([]byte, int)) error {
	r := bytes.NewReader(p.RawData)
	switch p.Format {
	case profile.FormatTrie:
		err := transporttrie.IterateRaw(r, make([]byte, 0, 256), cb)
		if err != nil {
			return err
		}
	case profile.FormatGroups:
		scanner := bufio.NewScanner(r)
		for scanner.Scan() {
			if err := scanner.Err(); err != nil {
				return err
			}
			line := scanner.Bytes()
			index := bytes.LastIndexByte(line, byte(' '))
			if index == -1 {
				continue
			}
			stacktrace := line[:index]
			count := line[index+1:]
			i, err := strconv.Atoi(string(count))
			if err != nil {
				return err
			}
			cb(stacktrace, i)
		}
	}
	return nil
}

func (p *Profile) extractProfileV2(meta *profile.Meta) func([]byte, int) {
	if p.group.Group == nil {
		p.group.Group = models.NewGroup(models.NewMetadata(), models.NewTags())
	}
	profileID := profile.GetProfileID(meta)
	p.group.Group.GetMetadata().Add("profileID", profileID)
	p.group.Group.GetMetadata().Add("dataType", "CallStack")
	p.group.Group.GetMetadata().Add("language", meta.SpyName)
	p.group.Group.GetMetadata().Add("type", profile.DetectProfileType(meta.Units.DetectValueType()))
	return func(k []byte, v int) {
		name, stack := extractNameAndStacks(k)
		stackID := strconv.FormatUint(xxhash.Sum64(k), 10)
		newProfile := models.NewProfile(name, stackID, stack, meta.StartTime.UnixNano(), meta.EndTime.UnixNano(), models.NewTags(), []*models.ProfileValue{
			models.NewProfileValue(meta.Units.DetectValueType(), string(meta.Units), string(meta.AggregationType), uint64(v)),
		})
		p.group.Events = append(p.group.Events, newProfile)
	}
}

func (p *Profile) extractProfileV1(meta *profile.Meta) func([]byte, int) {
	profileID := profile.GetProfileID(meta)
	labels, _ := json.Marshal(meta.Key.Labels())
	labelStr := string(labels)

	return func(k []byte, v int) {
		name, stack := extractNameAndStacks(k)

		var content []*protocol.Log_Content
		content = append(content,
			&protocol.Log_Content{
				Key:   "name",
				Value: name,
			},
			&protocol.Log_Content{
				Key:   "stack",
				Value: strings.Join(stack, "\n"),
			},
			&protocol.Log_Content{
				Key:   "stackID",
				Value: strconv.FormatUint(xxhash.Sum64(k), 10),
			},
			&protocol.Log_Content{
				Key:   "language",
				Value: meta.SpyName,
			},
			&protocol.Log_Content{
				Key:   "type",
				Value: profile.DetectProfileType(meta.Units.DetectValueType()),
			},
			&protocol.Log_Content{
				Key:   "units",
				Value: string(meta.Units),
			},
			&protocol.Log_Content{
				Key:   "valueTypes",
				Value: meta.Units.DetectValueType(),
			},
			&protocol.Log_Content{
				Key:   "aggTypes",
				Value: string(meta.AggregationType),
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
				Value: profileID,
			},
			&protocol.Log_Content{
				Key:   "labels",
				Value: labelStr,
			},
			&protocol.Log_Content{
				Key:   "value_0",
				Value: strconv.Itoa(v),
			},
		)

		log := &protocol.Log{
			Time:     uint32(meta.StartTime.Unix()),
			Contents: content,
		}
		p.logs = append(p.logs, log)
	}

}

func extractNameAndStacks(k []byte) (name string, stack []string) {
	slice := strings.Split(string(k), ";")
	if len(slice) > 0 && slice[len(slice)-1] == "" {
		slice = slice[:len(slice)-1]
	}
	if len(slice) == 1 {
		return slice[0], []string{}
	}
	name = slice[len(slice)-1]
	slice = slice[:len(slice)-1]
	helper.ReverseStringSlice(slice)
	return name, slice
}
