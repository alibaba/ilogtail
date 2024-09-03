package kubernetesmetav2

import (
	"encoding/json"
	"strconv"
	"strings"
	"time"

	v1 "k8s.io/api/core/v1"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/models"
)

func (m *metaCollector) processEntityCommonPart(logContents models.LogContents, kind, namespace, name, method string, firstObservedTime, lastObservedTime, creationTime int64) {
	// entity reserved fields
	logContents.Add(entityDomainFieldName, m.serviceK8sMeta.Domain)
	logContents.Add(entityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(kind))
	logContents.Add(entityIDFieldName, m.genKey(namespace, name))
	logContents.Add(entityMethodFieldName, method)

	logContents.Add(entityFirstObservedTimeFieldName, strconv.FormatInt(firstObservedTime, 10))
	logContents.Add(entityLastObservedTimeFieldName, strconv.FormatInt(lastObservedTime, 10))
	logContents.Add(entityKeepAliveSecondsFieldName, strconv.FormatInt(int64(m.serviceK8sMeta.Interval*2), 10))
	logContents.Add(entityCategoryFieldName, defaultEntityCategory)

	// common custom fields
	logContents.Add(entityClusterIDFieldName, m.serviceK8sMeta.clusterID)
	logContents.Add(entityKindFieldName, kind)
	logContents.Add(entityNameFieldName, name)
	logContents.Add(entityCreationTimeFieldName, strconv.FormatInt(creationTime, 10))
}

func (m *metaCollector) processPodEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	result := []models.PipelineEvent{}
	if obj, ok := data.Raw.(*v1.Pod); ok {
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
		log.Contents.Add("status", string(obj.Status.Phase))
		log.Contents.Add("instance_ip", obj.Status.PodIP)
		containerInfos := []map[string]string{}
		for _, container := range obj.Spec.Containers {
			containerInfo := map[string]string{
				"name":  container.Name,
				"image": container.Image,
			}
			containerInfos = append(containerInfos, containerInfo)
		}
		containersStr, _ := json.Marshal(containerInfos)
		log.Contents.Add("containers", string(containersStr))
		result = append(result, log)

		// container
		if m.serviceK8sMeta.Container {
			for _, container := range obj.Spec.Containers {
				containerLog := &models.Log{}
				containerLog.Contents = models.NewLogContents()
				containerLog.Timestamp = log.Timestamp

				containerLog.Contents.Add(entityDomainFieldName, m.serviceK8sMeta.Domain)
				containerLog.Contents.Add(entityTypeFieldName, k8sEntityTypePrefix+"container")
				containerLog.Contents.Add(entityIDFieldName, m.genKey(obj.Namespace, obj.Name+container.Name))
				containerLog.Contents.Add(entityMethodFieldName, method)

				containerLog.Contents.Add(entityFirstObservedTimeFieldName, strconv.FormatInt(data.FirstObservedTime, 10))
				containerLog.Contents.Add(entityLastObservedTimeFieldName, strconv.FormatInt(data.LastObservedTime, 10))
				containerLog.Contents.Add(entityKeepAliveSecondsFieldName, strconv.FormatInt(int64(m.serviceK8sMeta.Interval*2), 10))
				containerLog.Contents.Add(entityCategoryFieldName, defaultEntityCategory)

				// common custom fields
				containerLog.Contents.Add(entityClusterIDFieldName, m.serviceK8sMeta.clusterID)
				containerLog.Contents.Add(entityNameFieldName, container.Name)

				// custom fields
				containerLog.Contents.Add("image", container.Image)
				containerLog.Contents.Add("cpu_request", container.Resources.Requests.Cpu().String())
				containerLog.Contents.Add("cpu_limit", container.Resources.Limits.Cpu().String())
				containerLog.Contents.Add("memory_request", container.Resources.Requests.Memory().String())
				containerLog.Contents.Add("memory_limit", container.Resources.Limits.Memory().String())
				containerLog.Contents.Add("container_port", strconv.Itoa(int(container.Ports[0].ContainerPort)))
				containerLog.Contents.Add("protocol", string(container.Ports[0].Protocol))
				result = append(result, containerLog)
			}
		}
	}
	return nil
}

func (m *metaCollector) processNodeEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.Node); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Timestamp = uint64(time.Now().Unix())
		m.processEntityCommonPart(log.Contents, obj.Kind, "", obj.Name, method, data.FirstObservedTime, data.LastObservedTime, obj.CreationTimestamp.Unix())

		// custom fields
		labelsStr, _ := json.Marshal(obj.Labels)
		log.Contents.Add("labels", string(labelsStr))
		annotationsStr, _ := json.Marshal(obj.Annotations)
		log.Contents.Add("annotations", string(annotationsStr))
		log.Contents.Add("status", string(obj.Status.Phase))
		for _, addr := range obj.Status.Addresses {
			if addr.Type == v1.NodeInternalIP {
				log.Contents.Add("internal_ip", addr.Address)
			} else if addr.Type == v1.NodeHostName {
				log.Contents.Add("host_name", addr.Address)
			}
		}
		capacityStr, _ := json.Marshal(obj.Status.Capacity)
		log.Contents.Add("capacity", string(capacityStr))
		allocatableStr, _ := json.Marshal(obj.Status.Allocatable)
		log.Contents.Add("allocatable", string(allocatableStr))
		addressStr, _ := json.Marshal(obj.Status.Addresses)
		log.Contents.Add("addresses", string(addressStr))
		log.Contents.Add("provider_id", obj.Spec.ProviderID)
		return []models.PipelineEvent{log}
	}
	return nil
}

func (m *metaCollector) processServiceEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.Service); ok {
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
		selectorStr, _ := json.Marshal(obj.Spec.Selector)
		log.Contents.Add("selector", string(selectorStr))
		log.Contents.Add("type", string(obj.Spec.Type))
		log.Contents.Add("cluster_ip", obj.Spec.ClusterIP)
		portsStr, _ := json.Marshal(obj.Spec.Ports)
		log.Contents.Add("ports", string(portsStr))
		return []models.PipelineEvent{log}
	}
	return nil
}

func (m *metaCollector) processConfigMapEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.ConfigMap); ok {
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

func (m *metaCollector) processSecretEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.Secret); ok {
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
		log.Contents.Add("type", string(obj.Type))

		return []models.PipelineEvent{log}
	}
	return nil
}

func (m *metaCollector) processNamespaceEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.Namespace); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Timestamp = uint64(time.Now().Unix())
		m.processEntityCommonPart(log.Contents, obj.Kind, "", obj.Name, method, data.FirstObservedTime, data.LastObservedTime, obj.CreationTimestamp.Unix())

		// custom fields
		log.Contents.Add("api_version", obj.APIVersion)
		log.Contents.Add("kind", obj.Kind)
		log.Contents.Add("name", obj.Name)
		for k, v := range obj.Labels {
			log.Contents.Add("label_"+k, v)
		}
		return []models.PipelineEvent{log}
	}
	return nil
}

func (m *metaCollector) processPersistentVolumeEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.PersistentVolume); ok {
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
		log.Contents.Add("status", string(obj.Status.Phase))
		log.Contents.Add("storage_class_name", obj.Spec.StorageClassName)
		log.Contents.Add("persistent_volume_reclaim_policy", string(obj.Spec.PersistentVolumeReclaimPolicy))
		log.Contents.Add("volume_mode", string(*obj.Spec.VolumeMode))
		log.Contents.Add("capacity", obj.Spec.Capacity.Storage().String())
		log.Contents.Add("fsType", obj.Spec.CSI.FSType)
		return []models.PipelineEvent{log}
	}
	return nil
}

func (m *metaCollector) processPersistentVolumeClaimEntity(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.PersistentVolumeClaim); ok {
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
		log.Contents.Add("status", string(obj.Status.Phase))
		log.Contents.Add("storeage_requests", obj.Spec.Resources.Requests.Storage().String())
		log.Contents.Add("storage_class_name", obj.Spec.StorageClassName)
		log.Contents.Add("volume_name", obj.Spec.VolumeName)
		return []models.PipelineEvent{log}
	}
	return nil
}

func (m *metaCollector) processPodNodeLink(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.NodePod); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Node.Kind))
		log.Contents.Add(entityLinkSrcEntityIDFieldName, m.genKey(obj.Pod.Namespace, obj.Pod.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkDestEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Pod.Kind))
		log.Contents.Add(entityLinkDestEntityIDFieldName, m.genKey("", obj.Node.Name))

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

func (m *metaCollector) processPodPVCLink(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.PodPersistentVolumeClaim); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Pod.Kind))
		log.Contents.Add(entityLinkSrcEntityIDFieldName, m.genKey(obj.Pod.Namespace, obj.Pod.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkDestEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.PersistentVolumeClaim.Kind))
		log.Contents.Add(entityLinkDestEntityIDFieldName, m.genKey(obj.PersistentVolumeClaim.Namespace, obj.PersistentVolumeClaim.Name))

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

func (m *metaCollector) processPodConfigMapLink(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.PodConfigMap); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Pod.Kind))
		log.Contents.Add(entityLinkSrcEntityIDFieldName, m.genKey(obj.Pod.Namespace, obj.Pod.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkDestEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.ConfigMap.Kind))
		log.Contents.Add(entityLinkDestEntityIDFieldName, m.genKey(obj.ConfigMap.Namespace, obj.ConfigMap.Name))

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

func (m *metaCollector) processPodSecretLink(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.PodSecret); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Pod.Kind))
		log.Contents.Add(entityLinkSrcEntityIDFieldName, m.genKey(obj.Pod.Namespace, obj.Pod.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkDestEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Secret.Kind))
		log.Contents.Add(entityLinkDestEntityIDFieldName, m.genKey(obj.Secret.Namespace, obj.Secret.Name))

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

func (m *metaCollector) processPodServiceLink(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.PodService); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Pod.Kind))
		log.Contents.Add(entityLinkSrcEntityIDFieldName, m.genKey(obj.Service.Namespace, obj.Service.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkDestEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Service.Kind))
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

func (m *metaCollector) processPodContainerLink(data *k8smeta.ObjectWrapper, method string) []models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.PodContainer); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, k8sEntityTypePrefix+strings.ToLower(obj.Pod.Kind))
		log.Contents.Add(entityLinkSrcEntityIDFieldName, m.genKey(obj.Pod.Namespace, obj.Pod.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, m.serviceK8sMeta.Domain)
		log.Contents.Add(entityLinkDestEntityTypeFieldName, k8sEntityTypePrefix+"container")
		log.Contents.Add(entityLinkDestEntityIDFieldName, m.genKey(obj.Pod.Namespace, obj.Container.Name))

		log.Contents.Add(entityLinkRelationTypeFieldName, "contains")
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
