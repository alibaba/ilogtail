package jfr

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"github.com/alibaba/ilogtail/pkg/models"
	"io"
	"mime/multipart"
	"strconv"
	"strings"

	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"google.golang.org/protobuf/proto"

	"github.com/pyroscope-io/pyroscope/pkg/util/form"
)

const (
	formFieldProfile, formFieldSampleTypeConfig = "profile", "sample_type_config"
)

type RawProfile struct {
	RawData             []byte
	FormDataContentType string

	logs  []*protocol.Log             // v1 result
	group *models.PipelineGroupEvents // v2 result
}

func (r *RawProfile) ParseV2(ctx context.Context, meta *profile.Meta) (groups *models.PipelineGroupEvents, err error) {
	reader, labels, err := r.extractProfileRaw()
	if err != nil {
		return nil, err
	}
	groups = new(models.PipelineGroupEvents)
	r.group = groups

	if err := r.ParseJFR(ctx, meta, reader, labels, r.extractProfileV2(meta)); err != nil {
		return nil, err
	}
	r.group = nil
	return groups, nil
}

func (r *RawProfile) Parse(ctx context.Context, meta *profile.Meta) (logs []*protocol.Log, err error) {
	reader, labels, err := r.extractProfileRaw()
	if err != nil {
		return nil, err
	}
	if err := r.ParseJFR(ctx, meta, reader, labels, r.extractProfileV1(meta)); err != nil {
		return nil, err
	}
	logs = r.logs
	r.logs = nil
	return logs, nil
}

func (r *RawProfile) extractProfileV1(meta *profile.Meta) profile.CallbackFunc {
	profileID := profile.GetProfileID(meta)
	return func(id uint64, stack *profile.Stack, vals []uint64, types, units, aggs []string, startTime, endTime int64, labels map[string]string) {
		var content []*protocol.Log_Content
		b, _ := json.Marshal(labels)
		content = append(content,
			&protocol.Log_Content{
				Key:   "name",
				Value: stack.Name,
			},
			&protocol.Log_Content{
				Key:   "stack",
				Value: strings.Join(stack.Stack, "\n"),
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
				Value: profile.DetectProfileType(types[0]),
			},
			&protocol.Log_Content{
				Key:   "units",
				Value: strings.Join(units, ","),
			},
			&protocol.Log_Content{
				Key:   "valueTypes",
				Value: strings.Join(types, ","),
			},
			&protocol.Log_Content{
				Key:   "aggTypes",
				Value: strings.Join(aggs, ","),
			},
			&protocol.Log_Content{
				Key:   "dataType",
				Value: "CallStack",
			},
			&protocol.Log_Content{
				Key:   "durationNs",
				Value: strconv.FormatInt(endTime-startTime, 10),
			},

			&protocol.Log_Content{
				Key:   "profileID",
				Value: profileID,
			},
			&protocol.Log_Content{
				Key:   "labels",
				Value: string(b),
			},
		)
		for i, v := range vals {
			content = append(content, &protocol.Log_Content{
				Key:   fmt.Sprintf("value_%d", i),
				Value: strconv.FormatUint(v, 10),
			})
		}
		log := &protocol.Log{
			Time:     uint32(meta.StartTime.Unix()),
			Contents: content,
		}
		r.logs = append(r.logs, log)
	}
}

func (r *RawProfile) extractProfileV2(meta *profile.Meta) profile.CallbackFunc {
	if r.group.Group == nil {
		r.group.Group = models.NewGroup(models.NewMetadata(), models.NewTags())
	}
	r.group.Group.GetTags().Add("profileID", profile.GetProfileID(meta))
	r.group.Group.GetTags().Add("dataType", "CallStack")
	r.group.Group.GetTags().Add("language", meta.SpyName)
	return func(id uint64, stack *profile.Stack, vals []uint64, types, units, aggs []string, startTime, endTime int64, labels map[string]string) {
		r.group.Group.GetTags().Add("type", profile.DetectProfileType(types[0]))
		var values models.ProfileValues
		for i, val := range vals {
			values = append(values, models.NewProfileValue(types[i], units[i], aggs[i], val))
		}
		newProfile := models.NewProfile(stack.Name, strconv.FormatUint(id, 10), stack.Stack, startTime, endTime, models.NewTagsWithMap(labels), values)
		r.group.Events = append(r.group.Events, newProfile)
	}
}

func (r *RawProfile) extractProfileRaw() (io.Reader, *LabelsSnapshot, error) {
	var reader io.Reader = bytes.NewReader(r.RawData)
	var err error
	labels := new(LabelsSnapshot)
	if strings.Contains(r.FormDataContentType, "multipart/form-data") {
		if reader, labels, err = loadJFRFromForm(reader, r.FormDataContentType); err != nil {
			return nil, nil, err
		}
	}
	return reader, labels, err
}

func loadJFRFromForm(r io.Reader, contentType string) (io.Reader, *LabelsSnapshot, error) {
	boundary, err := form.ParseBoundary(contentType)
	if err != nil {
		return nil, nil, err
	}

	f, err := multipart.NewReader(r, boundary).ReadForm(32 << 20)
	if err != nil {
		return nil, nil, err
	}
	defer func() {
		_ = f.RemoveAll()
	}()

	jfrField, err := form.ReadField(f, "jfr")
	if err != nil {
		return nil, nil, err
	}
	if jfrField == nil {
		return nil, nil, fmt.Errorf("jfr field is required")
	}

	labelsField, err := form.ReadField(f, "labels")
	if err != nil {
		return nil, nil, err
	}
	var labels LabelsSnapshot
	if len(labelsField) > 0 {
		if err = proto.Unmarshal(labelsField, &labels); err != nil {
			return nil, nil, err
		}
	}

	return bytes.NewReader(jfrField), &labels, nil
}
