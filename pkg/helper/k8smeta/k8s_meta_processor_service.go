package k8smeta

import (
	"context"
	"fmt"
	"time"

	v1 "k8s.io/api/core/v1"
	"k8s.io/client-go/informers"
	"k8s.io/client-go/tools/cache"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type serviceProcessor struct {
	metaStore   *DeferredDeletionMetaStore
	metaManager *MetaManager

	eventCh    chan *K8sMetaEvent
	pipelineCh chan *K8sMetaEvent
	stopCh     chan struct{}
}

func (m *serviceProcessor) init(stopCh chan struct{}, pipelineCh chan *K8sMetaEvent) {
	idxRules := []IdxFunc{
		generateServiceNameKey,
	}
	m.eventCh = make(chan *K8sMetaEvent, 100)
	m.stopCh = stopCh
	m.pipelineCh = pipelineCh
	store := NewDeferredDeletionMetaStore(m.eventCh, m.stopCh, 60, 120, cache.MetaNamespaceKeyFunc, idxRules...)
	store.Start(m.flushPeriodEvent)
	m.metaStore = store
	m.watch(m.stopCh)
}

func (m *serviceProcessor) Get(key []string) map[string][]*K8sMetaEvent {
	return m.metaStore.Get(key)
}

func generateServiceNameKey(obj interface{}) ([]string, error) {
	service, ok := obj.(*v1.Service)
	if !ok {
		return []string{}, fmt.Errorf("object is not a service")
	}
	return []string{service.Name}, nil
}

func (m *serviceProcessor) watch(stopCh <-chan struct{}) {
	factory := informers.NewSharedInformerFactory(m.metaManager.clientset, time.Hour*1)
	informer := factory.Core().V1().Services().Informer()
	informer.AddEventHandler(cache.ResourceEventHandlerFuncs{
		AddFunc: func(obj interface{}) {
			nowTime := time.Now().Unix()
			m.flushRealTimeEvent(&K8sMetaEvent{
				EventType:    EventTypeAdd,
				ResourceType: SERVICE,
				RawObject:    obj,
				CreateTime:   nowTime,
				UpdateTime:   nowTime,
			})
		},
		UpdateFunc: func(oldObj interface{}, obj interface{}) {
			nowTime := time.Now().Unix()
			m.flushRealTimeEvent(&K8sMetaEvent{
				EventType:    EventTypeUpdate,
				ResourceType: SERVICE,
				RawObject:    obj,
				CreateTime:   nowTime,
				UpdateTime:   nowTime,
			})
		},
		DeleteFunc: func(obj interface{}) {
			m.flushRealTimeEvent(&K8sMetaEvent{
				EventType:    EventTypeDelete,
				ResourceType: SERVICE,
				RawObject:    obj,
			})
		},
	})
	go factory.Start(stopCh)
	// wait infinite for first cache sync success
	for {
		if !cache.WaitForCacheSync(stopCh, informer.HasSynced) {
			logger.Error(context.Background(), "K8S_META_CACHE_SYNC_TIMEOUT", "service cache sync timeout")
			time.Sleep(1 * time.Second)
		} else {
			break
		}
	}
}

func (m *serviceProcessor) flushRealTimeEvent(event *K8sMetaEvent) {
	// flush to cache
	m.eventCh <- event
	// flush to store pipeline, if block, discard and continue
	select {
	case m.pipelineCh <- event:
	default:
		logger.Error(context.Background(), "ENTITY_PIPELINE_QUEUE_FULL", "pipelineCh is full, discard in current sync")
	}
}

func (m *serviceProcessor) flushPeriodEvent(events []*K8sMetaEvent) {
	// flush to store pipeline, if block, discard and continue
	for _, event := range events {
		select {
		case m.pipelineCh <- event:
		default:
			logger.Error(context.Background(), "ENTITY_PIPELINE_QUEUE_FULL", "pipelineCh is full, discard in current sync")
		}
	}
}
