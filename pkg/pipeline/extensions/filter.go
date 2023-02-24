package extensions

import "github.com/alibaba/ilogtail/pkg/models"

type Filter interface {
	Filter(group *models.PipelineGroupEvents) *models.PipelineGroupEvents
}
