package k8smeta

import (
	"context"
	"sync"
	"time"

	v1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/labels"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/watch"
	"k8s.io/client-go/tools/cache"
)

type podProcessor struct {
	metaStore   *DeferredDeletionMetaStore
	metaManager *MetaManager

	serviceMetaStore *DeferredDeletionMetaStore

	podServiceHandlers map[string]HandlerFunc
	podServiceLock     sync.RWMutex
}

func (m *podProcessor) init() {
	m.metaStore = &DeferredDeletionMetaStore{
		cacheMetaStore: cacheMetaStore{
			addHandlers:    make(map[string]HandlerFunc),
			updateHandlers: map[string]HandlerFunc{},
			deleteHandlers: map[string]HandlerFunc{},
			keyFunc:        cache.MetaNamespaceKeyFunc,
			Items:          make(map[string]*ObjectWrapper),
		},
		refreshInterval: 60,
		gracePeriod:     120,
		deleteKeys:      make(map[string]struct{}),
	}
	m.podServiceHandlers = make(map[string]HandlerFunc)
}

func (m *podProcessor) watch(stopCh <-chan struct{}) {
	lw := &cache.ListWatch{
		ListFunc: func(options metav1.ListOptions) (runtime.Object, error) {
			return m.metaManager.clientset.CoreV1().Pods(metav1.NamespaceAll).List(context.Background(), options)
		},
		WatchFunc: func(options metav1.ListOptions) (watch.Interface, error) {
			return m.metaManager.clientset.CoreV1().Pods(metav1.NamespaceAll).Watch(context.Background(), options)
		},
	}

	reflector := cache.NewReflector(lw, &v1.Pod{}, m.metaStore, 0)
	go reflector.Run(stopCh)
	go m.timerFetchPods()
}

// 定时从cache中全量输出一次
func (m *podProcessor) timerFetchPods() error {
	interval := time.Second * time.Duration(refreshInterval)
	ticker := time.NewTicker(interval)
	for range ticker.C {
		objList := m.metaStore.ListObjWrapper()
		for _, obj := range objList {
			m.metaStore.AddWithoutCache(obj)
		}

		m.PodServiceProcess(objList)
	}
	return nil
}

func (m *podProcessor) RegisterHandlers(addHandler, updateHandler, deleteHandler HandlerFunc, configName string) {
	m.metaStore.RegisterHandlers(addHandler, updateHandler, deleteHandler, configName)
}

func (m *podProcessor) UnregisterHandlers(configName string) {
	m.metaStore.UnregisterHandlers(configName)
}

func (m *podProcessor) RegisterPodServiceHandlers(podServiceHandler HandlerFunc, configName string) {
	m.podServiceLock.Lock()
	defer m.podServiceLock.Unlock()
	m.podServiceHandlers[configName] = podServiceHandler
}

func (m *podProcessor) UnregisterPodServiceHandlers(configName string) {
	m.podServiceLock.Lock()
	defer m.podServiceLock.Unlock()
	delete(m.podServiceHandlers, configName)
}

func (m *podProcessor) PodServiceProcess(podList []*ObjectWrapper) {
	serviceList := m.serviceMetaStore.ListObjWrapper()

	results := make([]*PodService, 0)

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
				results = append(results, &PodService{
					Pod:     pod,
					Service: s.obj.(*v1.Service),
				})
			}
		}
	}
	for _, res := range results {
		m.podServiceLock.RLock()
		defer m.podServiceLock.RUnlock()
		for _, handler := range m.podServiceHandlers {
			nowTime := time.Now().Unix()
			handler(&ObjectWrapper{
				RawObject:  res,
				IsDeleted:  false,
				CreateTime: nowTime,
				UpdateTime: nowTime,
			})
		}
	}
}
