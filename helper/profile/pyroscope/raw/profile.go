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
	logs    []*protocol.Log             // v1 result
	group   *models.PipelineGroupEvents // v2 result
}

func (p *Profile) ParseV2(ctx context.Context, meta *profile.Meta) (groups *models.PipelineGroupEvents, err error) {
	groups = new(models.PipelineGroupEvents)
	p.group = groups
	cb := p.extractProfileV2(meta)
	if err := p.doParse(cb); err != nil {
		return nil, err
	}
	p.group = nil
	return
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
	return func(k []byte, v int) {
		name, stack := extractNameAndStacks(k)
		stackID := strconv.FormatUint(xxhash.Sum64(k), 16)
		newProfile := models.NewProfile(name, stackID,
			profileID, "CallStack", meta.SpyName, profile.DetectProfileType(meta.Units.DetectValueType()),
			stack, meta.StartTime.UnixNano(), meta.EndTime.UnixNano(), models.NewTags(), []*models.ProfileValue{
				models.NewProfileValue(meta.Units.DetectValueType(), string(meta.Units), string(meta.AggregationType), float64(v)),
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
				Value: strconv.FormatUint(xxhash.Sum64(k), 16),
			},
			&protocol.Log_Content{
				Key:   "language",
				Value: meta.SpyName,
			},
			&protocol.Log_Content{
				Key:   "type",
				Value: profile.DetectProfileType(meta.Units.DetectValueType()).String(),
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
