package extensions

import "github.com/alibaba/ilogtail/pkg/models"

type FlushInterceptor interface {
	Intercept(group *models.PipelineGroupEvents) *models.PipelineGroupEvents
}
