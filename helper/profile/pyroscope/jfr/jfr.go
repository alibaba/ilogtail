package jfr

import (
	"bytes"
	"context"
	"fmt"
	"io"
	"mime/multipart"
	"strings"

	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"google.golang.org/protobuf/proto"

	"github.com/pyroscope-io/pyroscope/pkg/util/form"
)

const (
	formFieldProfile, formFieldSampleTypeConfig = "profile", "sample_type_config"
	splitor                                     = "$@$"
)

type RawProfile struct {
	RawData             []byte
	FormDataContentType string
}

func (r *RawProfile) Parse(ctx context.Context, meta *profile.Meta) (logs []*protocol.Log, err error) {
	var reader io.Reader = bytes.NewReader(r.RawData)
	labels := new(LabelsSnapshot)
	if strings.Contains(r.FormDataContentType, "multipart/form-data") {
		if reader, labels, err = loadJFRFromForm(reader, r.FormDataContentType); err != nil {
			return nil, err
		}
	}
	return ParseJFR(ctx, meta, reader, labels)
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
