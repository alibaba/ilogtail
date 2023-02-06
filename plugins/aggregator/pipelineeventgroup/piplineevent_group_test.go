package pipelineeventgroup

import (
	_ "github.com/alibaba/ilogtail/pkg/logger/test"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/plugins/test/mock"

	"github.com/stretchr/testify/require"

	"testing"
)

func TestPipelineeventGroup_Record_Directly(t *testing.T) {
	p := new(PipelineeventGroup)
	p.LimitSize = 5
	ctx := pipeline.NewObservePipelineConext(100)
	p.Init(mock.NewEmptyContext("a", "b", "c"), nil)

	err := p.Record(constructEvents(104, map[string]string{}, map[string]string{}), ctx)
	require.NoError(t, err)
	require.Equal(t, 20, len(ctx.Collector().ToArray()))
}

func TestPipelineeventGroup_Record_Tag(t *testing.T) {
	p := new(PipelineeventGroup)
	p.LimitSize = 5
	p.ReserveTags = []string{"tag"}
	p.ReserveMetas = []string{"meta"}
	events := constructEvents(104, map[string]string{
		"meta": "metaval",
	}, map[string]string{
		"tag": "tagval",
	})
	ctx := pipeline.NewObservePipelineConext(100)
	p.Init(mock.NewEmptyContext("a", "b", "c"), nil)
	err := p.Record(events, ctx)
	require.NoError(t, err)
	array := ctx.Collector().ToArray()
	require.Equal(t, 20, len(array))
	require.Equal(t, 1, len(array[0].Group.Metadata.Iterator()))
	require.Equal(t, "metaval", array[0].Group.Metadata.Get("meta"))
	require.Equal(t, 1, len(array[0].Group.Tags.Iterator()))
	require.Equal(t, "tagval", array[0].Group.Tags.Get("tag"))
}

func TestPipelineeventGroup_Record_Timer(t *testing.T) {
	p := new(PipelineeventGroup)
	p.LimitSize = 500
	ctx := pipeline.NewObservePipelineConext(100)
	p.Init(mock.NewEmptyContext("a", "b", "c"), nil)
	err := p.Record(constructEvents(104, map[string]string{}, map[string]string{}), ctx)
	require.NoError(t, err)
	require.Equal(t, 0, len(ctx.Collector().ToArray()))
	_ = p.GetResult(ctx)
	array := ctx.Collector().ToArray()
	require.Equal(t, 1, len(array))
	require.Equal(t, 104, len(array[0].Events))
}

func constructEvents(num int, meta, tags map[string]string) *models.PipelineGroupEvents {
	group := models.NewGroup(models.NewMetadataWithMap(meta), models.NewTagsWithMap(tags))
	e := new(models.PipelineGroupEvents)
	e.Group = group
	for i := 0; i < num; i++ {
		e.Events = append(e.Events, &models.Profile{})
	}
	return e
}
