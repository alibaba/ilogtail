package kubernetesmetav2

import (
	"encoding/json"
	"strconv"
	"strings"
	"time"

	app "k8s.io/api/apps/v1"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/models"
)

func (m *metaCollector) processDeploymentEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*app.Deployment); ok {
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
		matchLabelsStr, _ := json.Marshal(obj.Spec.Selector.MatchLabels)
		log.Contents.Add("match_labels", string(matchLabelsStr))
		log.Contents.Add("replicas", strconv.FormatInt(int64(*obj.Spec.Replicas), 10))
		log.Contents.Add("ready_replicas", strconv.FormatInt(int64(obj.Status.ReadyReplicas), 10))
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

		return []models.PipelineEvent{log}
	}
	return nil
}

func (m *metaCollector) processDaemonSetEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*app.DaemonSet); ok {
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
		matchLabelsStr, _ := json.Marshal(obj.Spec.Selector.MatchLabels)
		log.Contents.Add("match_labels", string(matchLabelsStr))
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
		return []models.PipelineEvent{log}
	}
	return nil
}

func (m *metaCollector) processStatefulSetEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*app.StatefulSet); ok {
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
		matchLabelsStr, _ := json.Marshal(obj.Spec.Selector.MatchLabels)
		log.Contents.Add("match_labels", string(matchLabelsStr))
		log.Contents.Add("replicas", strconv.FormatInt(int64(*obj.Spec.Replicas), 10))
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
		return []models.PipelineEvent{log}
	}
	return nil
}

func (m *metaCollector) processReplicaSetEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*app.ReplicaSet); ok {
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
		matchLabelsStr, _ := json.Marshal(obj.Spec.Selector.MatchLabels)
		log.Contents.Add("match_labels", string(matchLabelsStr))
		log.Contents.Add("replicas", strconv.FormatInt(int64(*obj.Spec.Replicas), 10))
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
		return []models.PipelineEvent{log}
	}
	return nil
}

func (m *metaCollector) processReplicaSetDeploymentLink(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.ReplicaSetDeployment); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.ReplicaSet.Kind))
		log.Contents.Add(entityLinkSrcEntityIDFieldName, m.genKey(obj.ReplicaSet.Namespace, obj.ReplicaSet.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkDestEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Deployment.Kind))
		log.Contents.Add(entityLinkDestEntityIDFieldName, m.genKey(obj.Deployment.Namespace, obj.Deployment.Name))

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

func (m *metaCollector) processPodReplicaSetLink(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.PodReplicaSet); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Pod.Kind))
		log.Contents.Add(entityLinkSrcEntityIDFieldName, m.genKey(obj.ReplicaSet.Namespace, obj.ReplicaSet.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkDestEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.ReplicaSet.Kind))
		log.Contents.Add(entityLinkDestEntityIDFieldName, m.genKey(obj.Pod.Namespace, obj.Pod.Name))

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

func (m *metaCollector) processPodStatefulSetLink(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.PodStatefulSet); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Pod.Kind))
		log.Contents.Add(entityLinkSrcEntityIDFieldName, m.genKey(obj.StatefulSet.Namespace, obj.StatefulSet.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkDestEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.StatefulSet.Kind))
		log.Contents.Add(entityLinkDestEntityIDFieldName, m.genKey(obj.Pod.Namespace, obj.Pod.Name))

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

func (m *metaCollector) processPodDaemonSetLink(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.PodDaemonSet); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Pod.Kind))
		log.Contents.Add(entityLinkSrcEntityIDFieldName, m.genKey(obj.DaemonSet.Namespace, obj.DaemonSet.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkDestEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.DaemonSet.Kind))
		log.Contents.Add(entityLinkDestEntityIDFieldName, m.genKey(obj.Pod.Namespace, obj.Pod.Name))

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
