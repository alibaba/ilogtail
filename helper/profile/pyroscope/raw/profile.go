package raw

import (
	"bufio"
	"bytes"
	"context"
	"encoding/json"
	"strconv"
	"strings"

	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/alibaba/ilogtail/pkg/protocol"

	"github.com/cespare/xxhash/v2"
	"github.com/gofrs/uuid"
	"github.com/pyroscope-io/pyroscope/pkg/structs/transporttrie"
)

type Profile struct {
	RawData []byte
	logs    []*protocol.Log
	Format  profile.Format
}

func (p *Profile) Parse(ctx context.Context, meta *profile.Meta) (logs []*protocol.Log, err error) {
	cb := p.extractProfileLog(meta)
	r := bytes.NewReader(p.RawData)
	switch p.Format {
	case profile.FormatTrie:
		err = transporttrie.IterateRaw(r, make([]byte, 0, 256), cb)
		if err != nil {
			return nil, err
		}
	case profile.FormatGroups:
		scanner := bufio.NewScanner(r)
		for scanner.Scan() {
			if err := scanner.Err(); err != nil {
				return nil, err
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
				return nil, err
			}
			cb(stacktrace, i)
		}
	}

	return p.logs, nil

}

func (p *Profile) extractProfileLog(meta *profile.Meta) func([]byte, int) {
	var profileIDStr string
	if meta.Key.HasProfileID() {
		profileIDStr, _ = meta.Key.ProfileID()
	} else {
		profileID, _ := uuid.NewV4()
		profileIDStr = profileID.String()
	}
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
				Value: stack,
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
				Value: meta.Units.DetectProfileType(),
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
				Value: profileIDStr,
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

func extractNameAndStacks(k []byte) (name string, stack string) {
	slice := strings.Split(string(k), ";")
	if len(slice) > 0 && slice[len(slice)-1] == "" {
		slice = slice[:len(slice)-1]
	}
	if len(slice) == 1 {
		return slice[0], ""
	}
	name = slice[len(slice)-1]
	slice = slice[:len(slice)-1]
	helper.ReverseStringSlice(slice)
	return name, strings.Join(slice, "\n")
}
