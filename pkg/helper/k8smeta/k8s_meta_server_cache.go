package k8smeta

import (
	"context"
	"strings"
	"sync"

	v1 "k8s.io/api/core/v1"
	"k8s.io/client-go/tools/cache"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type WatchCache struct {
	cache      map[string]string
	cacheMutex sync.RWMutex

	deleteMutex sync.Mutex

	keyFunc cache.KeyFunc

	podStore K8sMetaStore
}

type podMetadata struct {
	Namespace    string            `json:"namespace"`
	WorkloadName string            `json:"workloadName"`
	WorkloadKind string            `json:"workloadKind"`
	ServiceName  string            `json:"serviceName"`
	Labels       map[string]string `json:"labels"`
	Images       map[string]string `json:"images"`
	IsDeleted    bool              `json:"-"`
}

func newHttpServerWatchCache(metaManager *MetaManager, store K8sMetaStore) *WatchCache {
	client := &WatchCache{
		cache:      make(map[string]string),
		cacheMutex: sync.RWMutex{},

		deleteMutex: sync.Mutex{},
		keyFunc:     cache.MetaNamespaceKeyFunc,
		podStore:    store,
	}
	metaManager.PodProcessor.RegisterHandlers(client.handlePodAdd, client.handlePodUpdate, nil, "meta_server")
	return client
}

func (w *WatchCache) getPodMetadata(identifiers []string) map[string]*podMetadata {
	metadatas := make(map[string]*podMetadata)
	keysMap := make(map[string]string)
	keys := make([]string, 0)
	w.cacheMutex.RLock()
	for _, identifier := range identifiers {
		if len(identifier) == 0 {
			continue
		}
		key, ok := w.cache[identifier]
		if !ok {
			continue
		}
		keysMap[key] = identifier
		keys = append(keys, key)
	}
	w.cacheMutex.RUnlock()

	res := w.podStore.ListObjWrapperByKeys(keys)
	for key, value := range res {
		if pod, ok := value.RawObject.(*v1.Pod); ok {
			images := make(map[string]string)
			for _, container := range pod.Spec.Containers {
				images[container.Name] = container.Image
			}
			podMetadata := &podMetadata{
				Namespace: pod.Namespace,
				Labels:    pod.Labels,
				Images:    images,
				IsDeleted: false,
			}
			if len(pod.GetOwnerReferences()) == 0 {
				podMetadata.WorkloadName = ""
				podMetadata.WorkloadKind = ""
				logger.Warning(context.Background(), "Pod has no owner", pod.Name)
			} else {
				podMetadata.WorkloadName = pod.GetOwnerReferences()[0].Name
				podMetadata.WorkloadKind = strings.ToLower(pod.GetOwnerReferences()[0].Kind)
			}
			identifiers := keysMap[key]
			metadatas[identifiers] = podMetadata
		}
	}
	return metadatas
}

func (w *WatchCache) handlePodAdd(data *ObjectWrapper) {
	if pod, ok := data.RawObject.(*v1.Pod); ok {
		w.addOrUpdatePod(pod)
	} else {
		logger.Error(context.Background(), "Failed to cast object to pod", data.RawObject)
	}
}

func (w *WatchCache) handlePodUpdate(data *ObjectWrapper) {
	if pod, ok := data.RawObject.(*v1.Pod); ok {
		w.addOrUpdatePod(pod)
	} else {
		logger.Error(context.Background(), "Failed to cast object to pod", data.RawObject)
	}
}

func (w *WatchCache) addOrUpdatePod(pod *v1.Pod) {
	podKey, err := w.keyFunc(pod)
	if err == nil {
		return
	}
	w.cacheMutex.Lock()
	defer w.cacheMutex.Unlock()
	if pod.Status.PodIP != "" {

		w.cache[pod.Status.PodIP] = podKey
	}
	for _, container := range pod.Status.ContainerStatuses {
		if container.ContainerID != "" {
			w.cache[container.ContainerID] = podKey
			logger.Debug(context.Background(), "Metadata Added", container.ContainerID)
		}
	}
}

func (w *WatchCache) handlePodDelete(data *ObjectWrapper) {
	pod, ok := data.RawObject.(*v1.Pod)
	if !ok {
		logger.Error(context.Background(), "Failed to cast object to pod", data.RawObject)
		return
	}
	w.cacheMutex.Lock()
	defer w.cacheMutex.Unlock()
	if pod.Status.PodIP != "" {
		delete(w.cache, pod.Status.PodIP)
	}
	for _, container := range pod.Status.ContainerStatuses {
		if container.ContainerID != "" {
			delete(w.cache, container.ContainerID)
		}
	}
}
