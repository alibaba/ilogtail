package kubernetesmetav2

import (
	"strconv"
	"time"

	app "k8s.io/api/apps/v1"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/models"
)

func (m *metaCollector) processDeploymentEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*app.Deployment); ok {
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
		for k, v := range obj.Labels {
			log.Contents.Add("labels_"+k, v)
		}
		for k, v := range obj.Annotations {
			log.Contents.Add("annotations_"+k, v)
		}
		for k, v := range obj.Spec.Selector.MatchLabels {
			log.Contents.Add("matchLabels_"+k, v)
		}
		log.Contents.Add("spec_replicas", strconv.FormatInt(int64(*obj.Spec.Replicas), 10))
		for i, container := range obj.Spec.Template.Spec.Containers {
			log.Contents.Add("container_"+strconv.FormatInt(int64(i), 10)+"_name", container.Name)
			log.Contents.Add("container_"+strconv.FormatInt(int64(i), 10)+"_image", container.Image)
		}
		return log
	}
	return nil
}

func (m *metaCollector) processDaemonSetEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*app.DaemonSet); ok {
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
		for k, v := range obj.Labels {
			log.Contents.Add("labels_"+k, v)
		}
		for k, v := range obj.Annotations {
			log.Contents.Add("annotations_"+k, v)
		}
		for k, v := range obj.Spec.Selector.MatchLabels {
			log.Contents.Add("matchLabels_"+k, v)
		}
		for i, container := range obj.Spec.Template.Spec.Containers {
			log.Contents.Add("container_"+strconv.FormatInt(int64(i), 10)+"_name", container.Name)
			log.Contents.Add("container_"+strconv.FormatInt(int64(i), 10)+"_image", container.Image)
		}
		return log
	}
	return nil
}

func (m *metaCollector) processStatefulSetEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*app.StatefulSet); ok {
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
		for k, v := range obj.Labels {
			log.Contents.Add("labels_"+k, v)
		}
		for k, v := range obj.Annotations {
			log.Contents.Add("annotations_"+k, v)
		}
		for k, v := range obj.Spec.Selector.MatchLabels {
			log.Contents.Add("matchLabels_"+k, v)
		}
		log.Contents.Add("spec_replicas", strconv.FormatInt(int64(*obj.Spec.Replicas), 10))
		for i, container := range obj.Spec.Template.Spec.Containers {
			log.Contents.Add("container_"+strconv.FormatInt(int64(i), 10)+"_name", container.Name)
			log.Contents.Add("container_"+strconv.FormatInt(int64(i), 10)+"_image", container.Image)
		}
		return log
	}
	return nil
}

func (m *metaCollector) processReplicaSetEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*app.ReplicaSet); ok {
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
		for k, v := range obj.Labels {
			log.Contents.Add("labels_"+k, v)
		}
		for k, v := range obj.Annotations {
			log.Contents.Add("annotations_"+k, v)
		}
		for k, v := range obj.Spec.Selector.MatchLabels {
			log.Contents.Add("matchLabels_"+k, v)
		}
		log.Contents.Add("spec_replicas", strconv.FormatInt(int64(*obj.Spec.Replicas), 10))
		for i, container := range obj.Spec.Template.Spec.Containers {
			log.Contents.Add("container_"+strconv.FormatInt(int64(i), 10)+"_name", container.Name)
			log.Contents.Add("container_"+strconv.FormatInt(int64(i), 10)+"_image", container.Image)
		}
		return log
	}
	return nil
}

func (m *metaCollector) processDeploymentReplicaSetLink(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.DeploymentReplicaSet); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, "k8s")
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, obj.Deployment.Kind)
		log.Contents.Add(entityLinkSrcEntityIDFieldName, genKey(obj.Deployment.Namespace, obj.Deployment.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, "k8s")
		log.Contents.Add(entityLinkDestEntityTypeFieldName, obj.ReplicaSet.Kind)
		log.Contents.Add(entityLinkDestEntityIDFieldName, genKey(obj.ReplicaSet.Namespace, obj.ReplicaSet.Name))

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

func (m *metaCollector) processReplicaSetPodLink(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.ReplicaSetPod); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, "k8s")
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, obj.ReplicaSet.Kind)
		log.Contents.Add(entityLinkSrcEntityIDFieldName, genKey(obj.ReplicaSet.Namespace, obj.ReplicaSet.Name))

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

func (m *metaCollector) processStatefulSetPodLink(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.StatefulSetPod); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, "k8s")
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, obj.StatefulSet.Kind)
		log.Contents.Add(entityLinkSrcEntityIDFieldName, genKey(obj.StatefulSet.Namespace, obj.StatefulSet.Name))

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

func (m *metaCollector) processDaemonSetPodLink(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.DaemonSetPod); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, "k8s")
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, obj.DaemonSet.Kind)
		log.Contents.Add(entityLinkSrcEntityIDFieldName, genKey(obj.DaemonSet.Namespace, obj.DaemonSet.Name))

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
