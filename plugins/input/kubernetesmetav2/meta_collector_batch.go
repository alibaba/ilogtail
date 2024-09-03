package kubernetesmetav2

import (
	"encoding/json"
	"strconv"
	"strings"
	"time"

	batch "k8s.io/api/batch/v1"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/models"
)

func (m *metaCollector) processJobEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*batch.Job); ok {
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
		log.Contents.Add("status", string(obj.Status.String()))
		containerInfos := []map[string]string{}
		for _, container := range obj.Spec.Template.Spec.Containers {
			containerInfo := map[string]string{
				"name":  container.Name,
				"image": container.Image,
			}
			containerInfos = append(containerInfos, containerInfo)
		}
		containersStr, _ := json.Marshal(containerInfos)
		log.Contents.Add("containers", string(containersStr))
		log.Contents.Add("suspend", strconv.FormatBool(*obj.Spec.Suspend))
		log.Contents.Add("backoff_limit", strconv.FormatInt(int64(*obj.Spec.BackoffLimit), 10))
		log.Contents.Add("completion", strconv.FormatInt(int64(*obj.Spec.Completions), 10))
		return []models.PipelineEvent{log}
	}
	return nil
}

func (m *metaCollector) processCronJobEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*batch.CronJob); ok {
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
		log.Contents.Add("schedule", obj.Spec.Schedule)
		log.Contents.Add("suspend", strconv.FormatBool(*obj.Spec.Suspend))
		return []models.PipelineEvent{log}
	}
	return nil
}

func (m *metaCollector) processJobCronJobLink(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.JobCronJob); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Job.Kind))
		log.Contents.Add(entityLinkSrcEntityIDFieldName, m.genKey(obj.Job.Namespace, obj.Job.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkDestEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.CronJob.Kind))
		log.Contents.Add(entityLinkDestEntityIDFieldName, m.genKey(obj.CronJob.Namespace, obj.CronJob.Name))

		log.Contents.Add(entityLinkRelationTypeFieldName, "related_to")
		log.Contents.Add(entityMethodFieldName, method)
		log.Contents.Add(entityFirstObservedTimeFieldName, strconv.FormatInt(data.FirstObservedTime, 10))
		log.Contents.Add(entityLastObservedTimeFieldName, strconv.FormatInt(data.LastObservedTime, 10))
		log.Contents.Add(entityKeepAliveSecondsFieldName, strconv.FormatInt(int64(m.serviceK8sMeta.Interval*2), 10))
		log.Contents.Add(entityCategoryFieldName, defaultEntityLinkCategory)
		log.Contents.Add(entityClusterIDFieldName, m.serviceK8sMeta.clusterID)
		log.Timestamp = uint64(time.Now().Unix())
		return []models.PipelineEvent{log}
	}
	return nil
}

func (m *metaCollector) processPodJobLink(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.PodJob); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Pod.Kind))
		log.Contents.Add(entityLinkSrcEntityIDFieldName, m.genKey(obj.Pod.Namespace, obj.Pod.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkDestEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Job.Kind))
		log.Contents.Add(entityLinkDestEntityIDFieldName, m.genKey(obj.Job.Namespace, obj.Job.Name))

		log.Contents.Add(entityLinkRelationTypeFieldName, "related_to")
		log.Contents.Add(entityMethodFieldName, method)

		log.Contents.Add(entityFirstObservedTimeFieldName, strconv.FormatInt(data.FirstObservedTime, 10))
		log.Contents.Add(entityLastObservedTimeFieldName, strconv.FormatInt(data.LastObservedTime, 10))
		log.Contents.Add(entityKeepAliveSecondsFieldName, strconv.FormatInt(int64(m.serviceK8sMeta.Interval*2), 10))
		log.Contents.Add(entityCategoryFieldName, defaultEntityLinkCategory)
		log.Contents.Add(entityClusterIDFieldName, m.serviceK8sMeta.clusterID)
		log.Timestamp = uint64(time.Now().Unix())
		return []models.PipelineEvent{log}
	}
	return nil
}
