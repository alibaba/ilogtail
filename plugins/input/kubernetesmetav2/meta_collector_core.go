package kubernetesmetav2

import (
	"encoding/json"
	"strconv"
	"time"

	v1 "k8s.io/api/core/v1"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/models"
)

func (m *metaCollector) processPodEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.Pod); ok {
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
			log.Contents.Add("label_"+k, v)
		}
		for k, v := range obj.Annotations {
			log.Contents.Add("annotations_"+k, v)
		}
		for i, container := range obj.Spec.Containers {
			log.Contents.Add("container_"+strconv.FormatInt(int64(i), 10)+"_name", container.Name)
			log.Contents.Add("container_"+strconv.FormatInt(int64(i), 10)+"_image", container.Image)
		}
		return log
	}
	return nil
}

func (m *metaCollector) processNodeEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.Node); ok {
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
		for i, addr := range obj.Status.Addresses {
			log.Contents.Add("node_address_"+strconv.FormatInt(int64(i), 10), addr.Address)
			log.Contents.Add("node_address_"+strconv.FormatInt(int64(i), 10)+"_type", string(addr.Type))
		}
		log.Contents.Add("node_cpu", obj.Status.Capacity.Cpu().String())
		log.Contents.Add("node_mem", obj.Status.Capacity.Memory().String())
		log.Contents.Add("node_pods", strconv.FormatInt(int64(obj.Status.Capacity.Pods().Value()), 10))
		log.Contents.Add("node_id", obj.Spec.ProviderID)
		log.Contents.Add("node_arch", obj.Status.NodeInfo.Architecture)
		log.Contents.Add("node_container_runtime", obj.Status.NodeInfo.ContainerRuntimeVersion)
		log.Contents.Add("node_kernel_version", obj.Status.NodeInfo.KernelVersion)
		log.Contents.Add("node_kubeProxy_version", obj.Status.NodeInfo.KubeProxyVersion)
		log.Contents.Add("node_kubelet_version", obj.Status.NodeInfo.KubeletVersion)
		log.Contents.Add("node_os", obj.Status.NodeInfo.OperatingSystem)
		log.Contents.Add("node_image", obj.Status.NodeInfo.OSImage)
		return log
	}
	return nil
}

func (m *metaCollector) processServiceEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.Service); ok {
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
			log.Contents.Add("label_"+k, v)
		}
		for k, v := range obj.Annotations {
			log.Contents.Add("annotations_"+k, v)
		}
		for k, v := range obj.Spec.Selector {
			log.Contents.Add("spec_selector_"+k, v)
		}
		log.Contents.Add("spec_type", string(obj.Spec.Type))
		log.Contents.Add("cluster_ip", obj.Spec.ClusterIP)
		for i, port := range obj.Spec.Ports {
			log.Contents.Add("ports_"+strconv.FormatInt(int64(i), 10)+"_port", strconv.FormatInt(int64(port.Port), 10))
			log.Contents.Add("ports_"+strconv.FormatInt(int64(i), 10)+"_targetPort", port.TargetPort.StrVal)
			log.Contents.Add("ports_"+strconv.FormatInt(int64(i), 10)+"_protocol", string(port.Protocol))
		}
		return log
	}
	return nil
}

func (m *metaCollector) processConfigMapEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.ConfigMap); ok {
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
		dataBytes, _ := json.Marshal(obj.Data)
		log.Contents.Add("data", string(dataBytes))
		return log
	}
	return nil
}

func (m *metaCollector) processSecretEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.Secret); ok {
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
		log.Contents.Add("type", string(obj.Type))

		return log
	}
	return nil
}

func (m *metaCollector) processNamespaceEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.Namespace); ok {
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
		for k, v := range obj.Labels {
			log.Contents.Add("label_"+k, v)
		}
		return log
	}
	return nil
}

func (m *metaCollector) processPersistentVolumeEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.PersistentVolume); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityDomainFieldName, "k8s")
		log.Contents.Add(entityTypeFieldName, obj.Kind)
		log.Contents.Add(entityIDFieldName, genKey("", obj.Name))
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
		log.Contents.Add("storageClassName", obj.Spec.StorageClassName)
		log.Contents.Add("capacity", obj.Spec.Capacity.Storage().String())
		log.Contents.Add("fsType", obj.Spec.CSI.FSType)
		return log
	}
	return nil
}

func (m *metaCollector) processPersistentVolumeClaimEntity(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*v1.PersistentVolumeClaim); ok {
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
		log.Contents.Add("storeage_requests", obj.Spec.Resources.Requests.Storage().String())
		log.Contents.Add("storageClassName", obj.Spec.StorageClassName)
		log.Contents.Add("volumeName", obj.Spec.VolumeName)
		return log
	}
	return nil
}

func (m *metaCollector) processNodePodLink(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.NodePod); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, "k8s")
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, obj.Pod.Kind)
		log.Contents.Add(entityLinkSrcEntityIDFieldName, genKey(obj.Pod.Namespace, obj.Pod.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, "k8s")
		log.Contents.Add(entityLinkDestEntityTypeFieldName, obj.Node.Kind)
		log.Contents.Add(entityLinkDestEntityIDFieldName, genKey("", obj.Node.Name))

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

func (m *metaCollector) processPodPVCLink(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.PodPersistentVolumeClaim); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, "k8s")
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, obj.Pod.Kind)
		log.Contents.Add(entityLinkSrcEntityIDFieldName, genKey(obj.Pod.Namespace, obj.Pod.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, "k8s")
		log.Contents.Add(entityLinkDestEntityTypeFieldName, obj.PersistentVolumeClaim.Kind)
		log.Contents.Add(entityLinkDestEntityIDFieldName, genKey(obj.PersistentVolumeClaim.Namespace, obj.PersistentVolumeClaim.Name))

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

func (m *metaCollector) processPodConfigMapLink(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.PodConfigMap); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, "k8s")
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, obj.Pod.Kind)
		log.Contents.Add(entityLinkSrcEntityIDFieldName, genKey(obj.Pod.Namespace, obj.Pod.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, "k8s")
		log.Contents.Add(entityLinkDestEntityTypeFieldName, obj.ConfigMap.Kind)
		log.Contents.Add(entityLinkDestEntityIDFieldName, genKey(obj.ConfigMap.Namespace, obj.ConfigMap.Name))

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

func (m *metaCollector) processPodSecretLink(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.PodSecret); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, "k8s")
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, obj.Pod.Kind)
		log.Contents.Add(entityLinkSrcEntityIDFieldName, genKey(obj.Pod.Namespace, obj.Pod.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, "k8s")
		log.Contents.Add(entityLinkDestEntityTypeFieldName, obj.Secret.Kind)
		log.Contents.Add(entityLinkDestEntityIDFieldName, genKey(obj.Secret.Namespace, obj.Secret.Name))

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

func (m *metaCollector) processServicePodLink(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.ServicePod); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, "k8s")
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, obj.Service.Kind)
		log.Contents.Add(entityLinkSrcEntityIDFieldName, genKey(obj.Service.Namespace, obj.Service.Name))

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

func (m *metaCollector) processPodContainerLink(data *k8smeta.ObjectWrapper, method string) models.PipelineEvent {
	if obj, ok := data.Raw.(*k8smeta.PodContainer); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add(entityLinkSrcDomainFieldName, "k8s")
		log.Contents.Add(entityLinkSrcEntityTypeFieldName, obj.Pod.Kind)
		log.Contents.Add(entityLinkSrcEntityIDFieldName, genKey(obj.Pod.Namespace, obj.Pod.Name))

		log.Contents.Add(entityLinkDestDomainFieldName, "k8s")
		log.Contents.Add(entityLinkDestEntityTypeFieldName, "Container")
		log.Contents.Add(entityLinkDestEntityIDFieldName, genKey(obj.Pod.Namespace, obj.Container.Name))

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
