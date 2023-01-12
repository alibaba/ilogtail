package tire

import (
	"bytes"
	"context"
	"encoding/json"
	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/cespare/xxhash/v2"
	"github.com/gofrs/uuid"
	"github.com/pyroscope-io/pyroscope/pkg/structs/transporttrie"
	"strconv"
	"strings"
)

type RawProfile struct {
	RawData []byte
	logs    []*protocol.Log
}

func (p *RawProfile) Parse(ctx context.Context, meta *profile.Meta) (logs []*protocol.Log, err error) {
	meta.SpyName = strings.TrimSuffix(meta.SpyName, "spy")
	cb := p.extractProfileLog(meta)
	r := bytes.NewReader(p.RawData)
	err = transporttrie.IterateRaw(r, make([]byte, 0, 256), cb)
	if err != nil {
		return nil, err
	}
	return p.logs, nil

}

func (p *RawProfile) extractProfileLog(meta *profile.Meta) func([]byte, int) {
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
				Key:   "__tag__:language",
				Value: meta.SpyName,
			},
			&protocol.Log_Content{
				Key:   "__tag__:type",
				Value: meta.Units.DetectProfileType(),
			},
			&protocol.Log_Content{
				Key:   "__tag__:units",
				Value: string(meta.Units),
			},
			&protocol.Log_Content{
				Key:   "__tag__:valueTypes",
				Value: meta.Units.DetectValueType(),
			},
			&protocol.Log_Content{
				Key:   "__tag__:aggTypes",
				Value: string(meta.AggregationType),
			},
			&protocol.Log_Content{
				Key:   "__tag__:dataType",
				Value: "CallStack",
			},
			&protocol.Log_Content{
				Key:   "__tag__:durationNs",
				Value: strconv.FormatInt(meta.EndTime.Sub(meta.StartTime).Nanoseconds(), 10),
			},
			&protocol.Log_Content{
				Key:   "__tag__:profileID",
				Value: profileIDStr,
			},
			&protocol.Log_Content{
				Key:   "__tag__:labels",
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
