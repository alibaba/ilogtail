package k8smeta

import (
	"context"
	"fmt"
	"time"

	v1 "k8s.io/api/core/v1"
	"k8s.io/apimachinery/pkg/labels"
	"k8s.io/client-go/informers"
	"k8s.io/client-go/tools/cache"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type podProcessor struct {
	metaStore        *DeferredDeletionMetaStore
	serviceMetaStore *DeferredDeletionMetaStore
	metaManager      *MetaManager

	eventCh    chan *K8sMetaEvent
	pipelineCh chan *K8sMetaEvent
	stopCh     chan struct{}
}

func (m *podProcessor) init(stopCh chan struct{}, pipelineCh chan *K8sMetaEvent) {
	idxRules := []IdxFunc{
		generatePodIPKey,
		generateContainerIDKey,
		generateHostIPKey,
	}
	m.eventCh = make(chan *K8sMetaEvent, 100)
	m.stopCh = stopCh
	m.pipelineCh = pipelineCh
	store := NewDeferredDeletionMetaStore(m.eventCh, m.stopCh, 60, 120, cache.MetaNamespaceKeyFunc, idxRules...)
	store.Start(m.flushPeriodEvent)
	m.metaStore = store
	m.watch(m.stopCh)
}

func (m *podProcessor) Get(key string) []*K8sMetaEvent {
	return m.metaStore.Get(key)
}

func generatePodIPKey(obj interface{}) ([]string, error) {
	pod, ok := obj.(*v1.Pod)
	if !ok {
		return []string{}, fmt.Errorf("object is not a pod")
	}
	return []string{pod.Status.PodIP}, nil
}

func generateContainerIDKey(obj interface{}) ([]string, error) {
	pod, ok := obj.(*v1.Pod)
	if !ok {
		return []string{}, fmt.Errorf("object is not a pod")
	}
	result := make([]string, len(pod.Status.ContainerStatuses))
	for i, containerStatus := range pod.Status.ContainerStatuses {
		result[i] = containerStatus.ContainerID
	}
	return result, nil
}

func generateHostIPKey(obj interface{}) ([]string, error) {
	pod, ok := obj.(*v1.Pod)
	if !ok {
		return []string{}, fmt.Errorf("object is not a pod")
	}
	return []string{pod.Status.HostIP}, nil
}

func (m *podProcessor) watch(stopCh <-chan struct{}) {
	factory := informers.NewSharedInformerFactory(m.metaManager.clientset, time.Hour*24)
	informer := factory.Core().V1().Pods().Informer()
	informer.AddEventHandler(cache.ResourceEventHandlerFuncs{
		AddFunc: func(obj interface{}) {
			nowTime := time.Now().Unix()
			m.flushRealTimeEvent(&K8sMetaEvent{
				EventType:    EventTypeAdd,
				ResourceType: POD,
				RawObject:    obj,
				CreateTime:   nowTime,
				UpdateTime:   nowTime,
			})
		},
		UpdateFunc: func(oldObj interface{}, obj interface{}) {
			nowTime := time.Now().Unix()
			m.flushRealTimeEvent(&K8sMetaEvent{
				EventType:    EventTypeUpdate,
				ResourceType: POD,
				RawObject:    obj,
				CreateTime:   nowTime,
				UpdateTime:   nowTime,
			})
		},
		DeleteFunc: func(obj interface{}) {
			m.flushRealTimeEvent(&K8sMetaEvent{
				EventType:    EventTypeDelete,
				ResourceType: POD,
				RawObject:    obj,
			})
		},
	})
	go factory.Start(stopCh)
	// wait infinite for first cache sync success
	for {
		if !cache.WaitForCacheSync(stopCh, informer.HasSynced) {
			logger.Error(context.Background(), "K8S_META_CACHE_SYNC_TIMEOUT", "pod cache sync timeout")
			time.Sleep(1 * time.Second)
		} else {
			break
		}
	}
}

func (m *podProcessor) flushRealTimeEvent(event *K8sMetaEvent) {
	// flush to cache
	m.eventCh <- event
	// flush to store pipeline, if block, discard and continue
	select {
	case m.pipelineCh <- event:
	default:
		logger.Error(context.Background(), "ENTITY_PIPELINE_QUEUE_FULL", "pipelineCh is full, discard in current sync")
	}
}

func (m *podProcessor) flushPeriodEvent(events []*K8sMetaEvent) {
	podServiceEvent := m.podServiceProcess(events)
	// flush to store pipeline, if block, discard and continue
	for _, event := range append(events, podServiceEvent...) {
		select {
		case m.pipelineCh <- event:
		default:
			logger.Error(context.Background(), "ENTITY_PIPELINE_QUEUE_FULL", "pipelineCh is full, discard in current sync")
		}
	}
}

func (m *podProcessor) podServiceProcess(podList []*K8sMetaEvent) []*K8sMetaEvent {
	serviceList := m.serviceMetaStore.List()
	results := make([]*K8sMetaEvent, 0)
	matchers := make(map[string]labelMatchers)
	for _, data := range serviceList {
		service, ok := data.RawObject.(*v1.Service)
		if !ok {
			continue
		}

		_, ok = matchers[service.Namespace]
		lm := newLabelMatcher(data.RawObject, labels.SelectorFromSet(service.Spec.Selector))
		if !ok {
			matchers[service.Namespace] = []*labelMatcher{lm}
		} else {
			matchers[service.Namespace] = append(matchers[service.Namespace], lm)
		}
	}

	for _, data := range podList {
		pod, ok := data.RawObject.(*v1.Pod)
		if !ok {
			continue
		}
		nsSelectors, ok := matchers[pod.Namespace]
		if !ok {
			continue
		}
		set := labels.Set(pod.Labels)
		for _, s := range nsSelectors {
			if !s.selector.Empty() && s.selector.Matches(set) {
				results = append(results, &K8sMetaEvent{
					EventType:    EventTypeUpdate,
					ResourceType: POD_SERVICE,
					RawObject: &PodService{
						Pod:     pod,
						Service: s.obj.(*v1.Service),
					},
				})
			}
		}
	}
	return results
}
