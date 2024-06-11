package kubernetesmeta

import (
	"context"
	"strings"
	"sync"
	"time"

	v1 "k8s.io/api/core/v1"
	"k8s.io/client-go/informers"
	"k8s.io/client-go/tools/cache"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type WatchClient struct {
	stopCh     chan struct{}
	cache      map[string]*podMetadata
	cacheMutex sync.RWMutex
	informer   cache.SharedInformer

	deleteQueue []deleteRequest
	deleteMutex sync.Mutex
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

type deleteRequest struct {
	key       string
	deletedAt time.Time
}

func NewWatchClient(factory informers.SharedInformerFactory, gracePeriod time.Duration, stopCh chan struct{}) *WatchClient {
	client := &WatchClient{
		stopCh:     stopCh,
		cache:      make(map[string]*podMetadata),
		cacheMutex: sync.RWMutex{},
		informer:   factory.Core().V1().Pods().Informer(),

		deleteQueue: make([]deleteRequest, 0),
		deleteMutex: sync.Mutex{},
	}
	client.informer.AddEventHandler(cache.ResourceEventHandlerFuncs{
		AddFunc:    client.handlePodAdd,
		UpdateFunc: client.handlePodUpdate,
		DeleteFunc: client.handlePodDelete,
	})
	go client.deleteLoop(30*time.Second, gracePeriod)
	return client
}

func (w *WatchClient) GetPodMetadata(identifiers []string) map[string]*podMetadata {
	metadatas := make(map[string]*podMetadata)
	w.cacheMutex.RLock()
	defer w.cacheMutex.RUnlock()
	for _, identifier := range identifiers {
		if identifier != "" {
			if v, ok := w.cache[identifier]; ok {
				metadatas[identifier] = v
				continue
			}
		}
	}
	return metadatas
}

func (w *WatchClient) handlePodAdd(obj interface{}) {
	if pod, ok := obj.(*v1.Pod); ok {
		w.addOrUpdatePod(pod)
	} else {
		logger.Error(context.Background(), "Failed to cast object to pod", obj)
	}
}

func (w *WatchClient) handlePodUpdate(oldObj, newObj interface{}) {
	if pod, ok := newObj.(*v1.Pod); ok {
		w.addOrUpdatePod(pod)
	} else {
		logger.Error(context.Background(), "Failed to cast object to pod", newObj)
	}
}

func (w *WatchClient) addOrUpdatePod(pod *v1.Pod) {
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
	w.cacheMutex.Lock()
	defer w.cacheMutex.Unlock()
	if pod.Status.PodIP != "" {
		w.cache[pod.Status.PodIP] = podMetadata
	}
	logger.Debug(context.Background(), "Metadata Added", pod.Status.PodIP)
	for _, container := range pod.Status.ContainerStatuses {
		if container.ContainerID != "" {
			w.cache[container.ContainerID] = podMetadata
			logger.Debug(context.Background(), "Metadata Added", container.ContainerID)
		}
	}
}

func (w *WatchClient) handlePodDelete(obj interface{}) {
	if pod, ok := obj.(*v1.Pod); ok {
		w.cacheMutex.Lock()
		w.cache[pod.Status.PodIP].IsDeleted = true
		for _, container := range pod.Status.ContainerStatuses {
			if container.ContainerID != "" {
				w.cache[container.ContainerID].IsDeleted = true
			}
		}
		w.cacheMutex.Unlock()

		w.deleteMutex.Lock()
		defer w.deleteMutex.Unlock()
		w.deleteQueue = append(w.deleteQueue, deleteRequest{
			key:       pod.Status.PodIP,
			deletedAt: time.Now(),
		})
		for _, container := range pod.Status.ContainerStatuses {
			if container.ContainerID != "" {
				w.deleteQueue = append(w.deleteQueue, deleteRequest{
					key:       container.ContainerID,
					deletedAt: time.Now(),
				})
			}
		}
	} else {
		logger.Error(context.Background(), "Failed to cast object to pod", obj)
	}
}

func (w *WatchClient) deleteLoop(interval time.Duration, gracePeriod time.Duration) {
	// This loop runs after N seconds and deletes pods from cache.
	// It iterates over the delete queue and deletes all that aren't
	// in the grace period anymore.
	for {
		select {
		case <-time.After(interval):
			var cutoff int
			now := time.Now()
			w.deleteMutex.Lock()
			for i, req := range w.deleteQueue {
				if req.deletedAt.Add(gracePeriod).After(now) {
					break
				}
				cutoff = i + 1
			}
			toDelete := w.deleteQueue[:cutoff]
			w.deleteQueue = w.deleteQueue[cutoff:]
			w.deleteMutex.Unlock()

			w.cacheMutex.Lock()
			for _, req := range toDelete {
				if value, ok := w.cache[req.key]; ok {
					if value.IsDeleted {
						delete(w.cache, req.key)
						logger.Debug(context.Background(), "Metadata Deleted: ", req.key)
					}
				}
			}
			w.cacheMutex.Unlock()
		case <-w.stopCh:
			return
		}
	}
}
