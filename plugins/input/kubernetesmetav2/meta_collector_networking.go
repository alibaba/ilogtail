package kubernetesmetav2

import (
	"encoding/json"
	"time"

	networking "k8s.io/api/networking/v1" //nolint:typecheck

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/models"
)

func (m *metaCollector) processIngressEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*networking.Ingress); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Timestamp = uint64(time.Now().Unix())
		m.processEntityCommonPart(log.Contents, obj.Kind, obj.Namespace, obj.Name, method, data.FirstObservedTime, data.LastObservedTime, obj.CreationTimestamp.Unix())

		// custom fields
		log.Contents.Add("api_version", obj.APIVersion)
		log.Contents.Add("namespace", obj.Namespace)
		labelsStr, _ := json.Marshal(obj.Labels)
		log.Contents.Add("labels", string(labelsStr))
		annotationsStr, _ := json.Marshal(obj.Annotations)
		log.Contents.Add("annotations", string(annotationsStr))
		return []models.PipelineEvent{log}
	}
	return nil
}
