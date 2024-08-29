package kubernetesmetav2

import (
	"strconv"
	"strings"
	"time"

	batch "k8s.io/api/batch/v1"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/models"
)

func (m *metaCollector) processJobEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*batch.Job); ok {
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
		log.Contents.Add("kind", obj.Kind)
		log.Contents.Add("name", obj.Name)
		log.Contents.Add("namespace", obj.Namespace)
		for i, container := range obj.Spec.Template.Spec.Containers {
			log.Contents.Add("container_"+strconv.FormatInt(int64(i), 10)+"_name", container.Name)
			log.Contents.Add("container_"+strconv.FormatInt(int64(i), 10)+"_image", container.Image)
		}
		log.Contents.Add("suspend", strconv.FormatBool(*obj.Spec.Suspend))
		log.Contents.Add("backoffLimit", strconv.FormatInt(int64(*obj.Spec.BackoffLimit), 10))
		log.Contents.Add("completion", strconv.FormatInt(int64(*obj.Spec.Completions), 10))
		return log
	}
	return nil
}

func (m *metaCollector) processCronJobEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*batch.CronJob); ok {
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
		log.Contents.Add("kind", obj.Kind)
		log.Contents.Add("name", obj.Name)
		log.Contents.Add("namespace", obj.Namespace)
		for i, container := range obj.Spec.JobTemplate.Spec.Template.Spec.Containers {
			log.Contents.Add("container_"+strconv.FormatInt(int64(i), 10)+"_name", container.Name)
			log.Contents.Add("container_"+strconv.FormatInt(int64(i), 10)+"_image", container.Image)
		}
		log.Contents.Add("schedule", obj.Spec.Schedule)
		log.Contents.Add("suspend", strconv.FormatBool(*obj.Spec.Suspend))
		return log
	}
	return nil
}

func (m *metaCollector) processJobCronJobLink(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
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
		return log
	}
	return nil
}

func (m *metaCollector) processPodJobLink(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
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
		return log
	}
	return nil
}
