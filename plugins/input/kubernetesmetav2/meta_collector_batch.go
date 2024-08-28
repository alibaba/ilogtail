package kubernetesmetav2

import (
	"strconv"
	"time"

	batch "k8s.io/api/batch/v1"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/models"
)

func (m *metaCollector) processJobEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*batch.Job); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityDomainFieldName, "k8s")
		log.Contents.Add(entityTypeFieldName, obj.Kind)
		log.Contents.Add(entityIDFieldName, genKey(obj.Namespace, obj.Name))
		log.Contents.Add(entityMethodFieldName, method)

		log.Contents.Add(entityFirstObservedTimeFieldName, strconv.FormatInt(data.FirstObservedTime, 10))
		log.Contents.Add(entityLastObservedTimeFieldName, strconv.FormatInt(data.LastObservedTime, 10))
		log.Contents.Add(entityKeepAliveSecondsFieldName, strconv.FormatInt(int64(m.serviceK8sMeta.Interval), 10))

		log.Contents.Add(entityCategoryFieldName, defaultEntityCategory)
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
		log.Contents.Add(entityDomainFieldName, "k8s")
		log.Contents.Add(entityTypeFieldName, obj.Kind)
		log.Contents.Add(entityIDFieldName, genKey(obj.Namespace, obj.Name))
		log.Contents.Add(entityMethodFieldName, method)

		log.Contents.Add(entityFirstObservedTimeFieldName, strconv.FormatInt(data.FirstObservedTime, 10))
		log.Contents.Add(entityLastObservedTimeFieldName, strconv.FormatInt(data.LastObservedTime, 10))
		log.Contents.Add(entityKeepAliveSecondsFieldName, strconv.FormatInt(int64(m.serviceK8sMeta.Interval), 10))

		log.Contents.Add(entityCategoryFieldName, defaultEntityCategory)
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

func (m *metaCollector) processCronJobJobLink(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.CronJobJob); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, "k8s")
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, obj.CronJob.Kind)
		log.Contents.Add(entityLinkSrcEntityIDFieldName, genKey(obj.CronJob.Namespace, obj.CronJob.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, "k8s")
		log.Contents.Add(entityLinkDestEntityTypeFieldName, obj.Job.Kind)
		log.Contents.Add(entityLinkDestEntityIDFieldName, genKey(obj.Job.Namespace, obj.Job.Name))

		log.Contents.Add(entityLinkRelationTypeFieldName, "link")

		switch method {
		case "create", "update":
			log.Contents.Add(entityMethodFieldName, "update")
		case "delete":
			log.Contents.Add(entityMethodFieldName, method)
		default:
			// 数据不完整就没有意义
			return nil
		}

		log.Contents.Add(entityFirstObservedTimeFieldName, strconv.FormatInt(data.FirstObservedTime, 10))
		log.Contents.Add(entityLastObservedTimeFieldName, strconv.FormatInt(data.LastObservedTime, 10))
		log.Contents.Add(entityKeepAliveSecondsFieldName, strconv.FormatInt(int64(m.serviceK8sMeta.Interval), 10))
		log.Contents.Add(entityCategoryFieldName, defaultEntityLinkCategory)
		log.Timestamp = uint64(time.Now().Unix())
		return log
	}
	return nil
}

func (m *metaCollector) processJobPodLink(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.JobPod); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, "k8s")
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, obj.Job.Kind)
		log.Contents.Add(entityLinkSrcEntityIDFieldName, genKey(obj.Job.Namespace, obj.Job.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, "k8s")
		log.Contents.Add(entityLinkDestEntityTypeFieldName, obj.Pod.Kind)
		log.Contents.Add(entityLinkDestEntityIDFieldName, genKey(obj.Pod.Namespace, obj.Pod.Name))

		log.Contents.Add(entityLinkRelationTypeFieldName, "link")

		switch method {
		case "create", "update":
			log.Contents.Add(entityMethodFieldName, "update")
		case "delete":
			log.Contents.Add(entityMethodFieldName, method)
		default:
			// 数据不完整就没有意义
			return nil
		}

		log.Contents.Add(entityFirstObservedTimeFieldName, strconv.FormatInt(data.FirstObservedTime, 10))
		log.Contents.Add(entityLastObservedTimeFieldName, strconv.FormatInt(data.LastObservedTime, 10))
		log.Contents.Add(entityKeepAliveSecondsFieldName, strconv.FormatInt(int64(m.serviceK8sMeta.Interval), 10))
		log.Contents.Add(entityCategoryFieldName, defaultEntityLinkCategory)
		log.Timestamp = uint64(time.Now().Unix())
		return log
	}
	return nil
}
