package collapsed

import (
	"context"
	"regexp"
	"strconv"
	"strings"

	"github.com/cespare/xxhash"

	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

const (
	formFieldProfile, formFieldSampleTypeConfig = "profile", "sample_type_config"
	splitor                                     = "$@$"
)

type RawProfile struct {
	RawData []byte
	regexp  *regexp.Regexp
}

func (r *RawProfile) Parse(ctx context.Context, meta *profile.Meta) ([]*protocol.Log, error) {
	logs := make([]*protocol.Log, 0)
	arrays := strings.Split(string(r.RawData), "\n")
	for _, data := range arrays {
		el, err := extractLogs(ctx, data, meta)
		if err == nil {
			logs = append(logs, el...)
		}
	}
	return logs, nil
}

func extractLogs(ctx context.Context, data string, meta *profile.Meta) ([]*protocol.Log, error) {
	if len(data) == 0 {
		return make([]*protocol.Log, 0), nil
	}
	logs := make([]*protocol.Log, 0)
	parttarn := regexp.MustCompile(`(.+) (\d)`)
	arrays := parttarn.FindStringSubmatch(data)
	s := arrays[1]
	v := arrays[2]
	stacks := strings.Split(s, ";")

	content := make([]*protocol.Log_Content, 0)

	fs := strings.Join(stacks, "\n")

	name := "unknown"
	if len(stacks) > 0 {
		name = stacks[0]
	}

	content = append(content,
		&protocol.Log_Content{
			Key:   "name",
			Value: name,
		},
		&protocol.Log_Content{
			Key:   "stack",
			Value: fs,
		},
		&protocol.Log_Content{
			Key:   "stackID",
			Value: strconv.FormatUint(xxhash.Sum64String(fs), 10),
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
			Value: "unknown",
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
			Value: "0",
		},
		&protocol.Log_Content{
			Key:   "__tag__:profileID",
			Value: "unknown",
		},
		&protocol.Log_Content{
			Key:   "__tag__:labels",
			Value: "",
		},
		&protocol.Log_Content{
			Key:   "value_0",
			Value: v,
		},
	)
	log := &protocol.Log{
		Time:     uint32(meta.StartTime.Unix()),
		Contents: content,
	}
	logs = append(logs, log)
	return logs, nil
}
