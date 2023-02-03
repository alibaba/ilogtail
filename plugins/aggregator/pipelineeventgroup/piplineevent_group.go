package pipelineeventgroup

import (
	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"

	"sync"
)

type PipelineeventGroup struct {
	context               ilogtail.Context
	defaultGroupInfo      *models.GroupInfo
	defaultPipelineEvents []models.PipelineEvent
	onceExtract           sync.Once
	currentLen            int

	ReserveTags  []string
	ReserveMetas []string
	LimitSize    int
}

func (p *PipelineeventGroup) Init(context ilogtail.Context, queue ilogtail.LogGroupQueue) (int, error) {
	p.context = context
	p.Reset()
	return 0, nil
}

func (p *PipelineeventGroup) Description() string {
	return "Pipeline event group aggregator with same group info"
}

func (p *PipelineeventGroup) Reset() {
	p.defaultPipelineEvents = make([]models.PipelineEvent, 0, p.LimitSize)
	p.currentLen = 0
}

func (p *PipelineeventGroup) Record(events *models.PipelineGroupEvents, context ilogtail.PipelineContext) error {
	// the pipeline event group aggregator designed for the data with same group info, which would aggregate small packets to a large packets.
	// so the extract group info would only extract once.
	if len(events.Events) == 0 {
		return nil
	}
	logger.Debug(p.context.GetRuntimeContext(), "input len", len(events.Events))
	p.onceExtract.Do(func() {
		if len(p.ReserveTags) == 0 && len(p.ReserveMetas) == 0 {
			p.defaultGroupInfo = models.NewGroup(models.NewMetadata(), models.NewTags())
			return
		}
		tags := make(map[string]string)
		metas := make(map[string]string)
		for _, k := range p.ReserveTags {
			v := events.Group.GetTags().Get(k)
			tags[k] = v
		}
		for _, k := range p.ReserveMetas {
			v := events.Group.GetMetadata().Get(k)
			metas[k] = v
		}
		p.defaultGroupInfo = models.NewGroup(models.NewMetadataWithMap(metas), models.NewTagsWithMap(tags))
	})
	// todo with size, currently only limit with length
	for {
		inputSize := len(events.Events)
		availableSize := p.LimitSize - p.currentLen
		if availableSize > 0 {
			if availableSize >= inputSize {
				p.defaultPipelineEvents = append(p.defaultPipelineEvents, events.Events...)
				p.currentLen += inputSize
				break
			} else if availableSize < inputSize {
				p.defaultPipelineEvents = append(p.defaultPipelineEvents, events.Events[0:availableSize]...)
				_ = p.GetResult(context)
				events.Events = events.Events[availableSize:]
			}
		}
	}
	logger.Debug(p.context.GetRuntimeContext(), "cache len", len(p.defaultPipelineEvents))
	return nil

}

func (p *PipelineeventGroup) GetResult(context ilogtail.PipelineContext) error {
	if len(p.defaultPipelineEvents) == 0 {
		return nil
	}
	logger.Debug(p.context.GetRuntimeContext(), "flush out len", len(p.defaultPipelineEvents))
	context.Collector().Collect(models.GroupDeepClone(p.defaultGroupInfo), p.defaultPipelineEvents...)
	p.Reset()
	return nil

}

func init() {
	ilogtail.Aggregators["aggregator_pipelineevent_group"] = func() ilogtail.Aggregator {
		return &PipelineeventGroup{
			LimitSize: 3000,
		}
	}
}
