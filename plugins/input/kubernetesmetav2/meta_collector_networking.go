package kubernetesmetav2

import (
	"strconv"
	"strings"
	"time"

	networking "k8s.io/api/networking/v1"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/models"
)

func (m *metaCollector) processIngressEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*networking.Ingress); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Kind))
		log.Contents.Add(entityIDFieldName, m.genKey(obj.Namespace, obj.Name))
		log.Contents.Add(entityMethodFieldName, method)

		log.Contents.Add(entityFirstObservedTimeFieldName, strconv.FormatInt(data.FirstObservedTime, 10))
		log.Contents.Add(entityLastObservedTimeFieldName, strconv.FormatInt(data.LastObservedTime, 10))
		log.Contents.Add(entityKeepAliveSecondsFieldName, strconv.FormatInt(int64(m.serviceK8sMeta.Interval*2), 10))
		log.Contents.Add(entityCategoryFieldName, defaultEntityCategory)
		log.Contents.Add(entityClusterIDFieldName, m.serviceK8sMeta.clusterID)
		log.Timestamp = uint64(time.Now().Unix())

		// custom fields
		log.Contents.Add("apiVersion", obj.APIVersion)
		log.Contents.Add("kind", "ingress")
		log.Contents.Add("name", obj.Name)
		log.Contents.Add("namespace", obj.Namespace)
		return log
	}
	return nil
}
