package k8smeta

import (
	"context"
	"time"

	corev1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/apimachinery/pkg/watch"
	"k8s.io/client-go/tools/cache"
)

type serviceProcessor struct {
	metaStore   *DeferredDeletionMetaStore
	metaManager *MetaManager
}

func (m *serviceProcessor) init() {
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
}

func (m *serviceProcessor) watch(stopCh <-chan struct{}) {
	lw := &cache.ListWatch{
		ListFunc: func(options metav1.ListOptions) (runtime.Object, error) {
			return m.metaManager.clientset.CoreV1().Services(metav1.NamespaceAll).List(context.Background(), options)
		},
		WatchFunc: func(options metav1.ListOptions) (watch.Interface, error) {
			return m.metaManager.clientset.CoreV1().Services(metav1.NamespaceAll).Watch(context.Background(), options)
		},
	}

	reflector := cache.NewReflector(lw, &corev1.Service{}, m.metaStore, 0)
	go reflector.Run(stopCh)
	go m.timerFetchPods()
}

// 定时从cache中全量输出一次
func (m *serviceProcessor) timerFetchPods() error {
	interval := time.Second * time.Duration(refreshInterval)
	ticker := time.NewTicker(interval)
	for range ticker.C {
		objList := m.metaStore.ListObjWrapper()
		for _, obj := range objList {
			m.metaStore.AddWithoutCache(obj)
		}
	}
	return nil
}

func (m *serviceProcessor) RegisterHandlers(addHandler, updateHandler, deleteHandler HandlerFunc, configName string) {
	m.metaStore.RegisterHandlers(addHandler, updateHandler, deleteHandler, configName)
}

func (m *serviceProcessor) UnregisterHandlers(configName string) {
	m.metaStore.UnregisterHandlers(configName)
}
