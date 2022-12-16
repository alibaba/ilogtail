package jfr

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"mime/multipart"

	"github.com/alibaba/ilogtail/helper/profile"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"google.golang.org/protobuf/proto"

	"github.com/pyroscope-io/pyroscope/pkg/util/form"
)

type RawProfile struct {
	RawData             []byte
	FormDataContentType string

	profile        []byte
	labelsSnapshot *LabelsSnapshot
}

func (r *RawProfile) Parse(ctx context.Context, meta *profile.Meta) (logs []*protocol.Log, err error) {
	if err = r.extractProfile(); err != nil {
		return nil, fmt.Errorf("cannot extract profile: %w", err)
	}
	if len(r.profile) == 0 {
		return nil, errors.New("empty profile")
	}

	return extractLogs(ctx)
}

func (r *RawProfile) extractProfile() error {
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

	jfrField, err := form.ReadField(f, "jfr")
	if err != nil {
		return err
	}
	if jfrField == nil {
		return fmt.Errorf("jfr field is required")
	}

	labelsField, err := form.ReadField(f, "labels")
	if err != nil {
		return err
	}
	var labels LabelsSnapshot
	if len(labelsField) > 0 {
		if err = proto.Unmarshal(labelsField, &labels); err != nil {
			return err
		}
		r.labelsSnapshot = &labels
	}
	r.profile = jfrField
	return nil
}

func extractLogs(ctx context.Context, meta *profile.Meta, profile []byte, labelsSnapshot *LabelsSnapshot,  logs []*protocol.Log) error {
	ParseJFR(ctx, meta, profile, labelsSnapshot, logs)
	return nil
}
