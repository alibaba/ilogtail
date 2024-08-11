package k8smeta

import (
	"context"
	"fmt"
	"time"

	v1 "k8s.io/api/core/v1"
	"k8s.io/apimachinery/pkg/labels"
	"k8s.io/client-go/informers"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/tools/cache"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type podProcessor struct {
	metaStore        *DeferredDeletionMetaStore
	serviceMetaStore *DeferredDeletionMetaStore
	clientset        *kubernetes.Clientset

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
	store.Start()
	m.metaStore = store
	m.watch(m.stopCh)
}

func (m *podProcessor) Get(key []string) map[string][]*ObjectWrapper {
	return m.metaStore.Get(key)
}

func (m *podProcessor) List(service bool) []*ObjectWrapper {
	objs := m.metaStore.List()
	if service {
		podServiceObjs := m.podServiceProcess(objs)
		objs = append(objs, podServiceObjs...)
	}
	return objs
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
	factory := informers.NewSharedInformerFactory(m.clientset, time.Hour*24)
	informer := factory.Core().V1().Pods().Informer()
	informer.AddEventHandler(cache.ResourceEventHandlerFuncs{
		AddFunc: func(obj interface{}) {
			nowTime := time.Now().Unix()
			m.flushRealTimeEvent(&K8sMetaEvent{
				EventType: EventTypeAdd,
				Object: &ObjectWrapper{
					ResourceType: POD,
					Raw:          obj,
					CreateTime:   nowTime,
					UpdateTime:   nowTime,
				},
			})
		},
		UpdateFunc: func(oldObj interface{}, obj interface{}) {
			nowTime := time.Now().Unix()
			m.flushRealTimeEvent(&K8sMetaEvent{
				EventType: EventTypeUpdate,
				Object: &ObjectWrapper{
					ResourceType: POD,
					Raw:          obj,
					CreateTime:   nowTime,
					UpdateTime:   nowTime,
				},
			})
		},
		DeleteFunc: func(obj interface{}) {
			m.flushRealTimeEvent(&K8sMetaEvent{
				EventType: EventTypeDelete,
				Object: &ObjectWrapper{
					ResourceType: POD,
					Raw:          obj,
				},
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

func (m *podProcessor) podServiceProcess(podList []*ObjectWrapper) []*ObjectWrapper {
	serviceList := m.serviceMetaStore.List()
	results := make([]*ObjectWrapper, 0)
	matchers := make(map[string]labelMatchers)
	for _, data := range serviceList {
		service, ok := data.Raw.(*v1.Service)
		if !ok {
			continue
		}

		_, ok = matchers[service.Namespace]
		lm := newLabelMatcher(data.Raw, labels.SelectorFromSet(service.Spec.Selector))
		if !ok {
			matchers[service.Namespace] = []*labelMatcher{lm}
		} else {
			matchers[service.Namespace] = append(matchers[service.Namespace], lm)
		}
	}

	for _, data := range podList {
		pod, ok := data.Raw.(*v1.Pod)
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
				results = append(results, &ObjectWrapper{
					ResourceType: POD_SERVICE,
					Raw: &PodService{
						Pod:     pod,
						Service: s.obj.(*v1.Service),
					},
				})
			}
		}
	}
	return results
}
