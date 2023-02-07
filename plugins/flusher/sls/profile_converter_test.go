package sls

import (
	"github.com/stretchr/testify/require"

	"testing"

	"github.com/alibaba/ilogtail/pkg/models"
)

func TestConvertProfile(t *testing.T) {
	events := models.PipelineGroupEvents{
		Group: models.NewGroup(models.NewMetadata(), models.NewTags()),
		Events: []models.PipelineEvent{
			models.NewProfile("name", "sid", "pid", "callstack", "go", models.ProfileMem, models.ProfileStack{"stack"}, 1, 2, models.NewTags(), models.ProfileValues{models.NewProfileValue("val_type", "val_unit", "val_agg", 12), models.NewProfileValue("val_type", "val_unit", "val_agg", 13)}),
		},
	}

	logs := ConvertProfile(&events)
	require.Equal(t, 2, len(logs.Logs))
	println(logs.Logs[0].String())
	println(logs.Logs[1].String())
}
