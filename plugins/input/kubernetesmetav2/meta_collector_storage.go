package kubernetesmetav2

import (
	"encoding/json"
	"time"

	storage "k8s.io/api/storage/v1" //nolint:typecheck

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/models"
)

func (m *metaCollector) processStorageClassEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*storage.StorageClass); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Timestamp = uint64(time.Now().Unix())
		m.processEntityCommonPart(log.Contents, obj.Kind, obj.Namespace, obj.Name, method, data.FirstObservedTime, data.LastObservedTime, obj.CreationTimestamp.Unix())

		// custom fields
		log.Contents.Add("api_version", obj.APIVersion)
		labelsStr, _ := json.Marshal(obj.Labels)
		log.Contents.Add("labels", string(labelsStr))
		annotationsStr, _ := json.Marshal(obj.Annotations)
		log.Contents.Add("annotations", string(annotationsStr))
		log.Contents.Add("reclaim_policy", string(*obj.ReclaimPolicy))
		log.Contents.Add("volume_binding_mode", string(*obj.VolumeBindingMode))
		return []models.PipelineEvent{log}
	}
	return nil
}
