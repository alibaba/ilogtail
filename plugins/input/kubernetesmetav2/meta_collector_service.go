package kubernetesmetav2

import (
	"strconv"
	"time"

	v1 "k8s.io/api/core/v1"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/models"
)

func (m *metaCollector) processServiceEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.Service); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityDomainFieldName, "k8s")
		log.Contents.Add(entityTypeFieldName, "service")
		log.Contents.Add(entityIDFieldName, genKeyByService(obj))
		log.Contents.Add(entityMethodFieldName, method)

		log.Contents.Add(entityFirstObservedTimeFieldName, strconv.FormatInt(data.FirstObservedTime, 10))
		log.Contents.Add(entityLastObservedTimeFieldName, strconv.FormatInt(data.LastObservedTime, 10))
		log.Contents.Add(entityKeepAliveSecondsFieldName, strconv.FormatInt(int64(m.serviceK8sMeta.Interval), 10))

		log.Contents.Add(entityNamespaceFieldName, obj.Namespace)
		log.Contents.Add(entityNameFieldName, obj.Name)
		log.Contents.Add(entityCategoryFieldName, defaultEntityCategory)
		log.Timestamp = uint64(time.Now().Unix())
		return log
	}
	return nil
}
