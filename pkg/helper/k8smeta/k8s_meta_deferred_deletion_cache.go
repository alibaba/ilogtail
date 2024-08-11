package k8smeta

import (
	"context"
	"sync"
	"time"

	"k8s.io/client-go/tools/cache"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type DeferredDeletionMetaStore struct {
	keyFunc    cache.KeyFunc
	indexRules []IdxFunc

	eventCh chan *K8sMetaEvent
	stopCh  <-chan struct{}

	// cache
	Items map[string]*ObjectWrapper
	Index map[string][]string
	lock  sync.RWMutex

	refreshInterval int64
	gracePeriod     int64
}

func NewDeferredDeletionMetaStore(eventCh chan *K8sMetaEvent, stopCh <-chan struct{}, refreshInterval, gracePeriod int64, keyFunc cache.KeyFunc, indexRules ...IdxFunc) *DeferredDeletionMetaStore {
	m := &DeferredDeletionMetaStore{
		keyFunc:    keyFunc,
		indexRules: indexRules,

		eventCh: eventCh,
		stopCh:  stopCh,

		Items: make(map[string]*ObjectWrapper),
		Index: make(map[string][]string),

		refreshInterval: refreshInterval,
		gracePeriod:     gracePeriod,
	}
	return m
}

func (m *DeferredDeletionMetaStore) Start() {
	go m.handleEvent()
}

func (m *DeferredDeletionMetaStore) Get(key []string) map[string][]*ObjectWrapper {
	m.lock.RLock()
	defer m.lock.RUnlock()
	result := make(map[string][]*ObjectWrapper)
	for _, k := range key {
		realKeys, ok := m.Index[k]
		if !ok {
			return nil
		}
		for _, realKey := range realKeys {
			result[k] = append(result[k], m.Items[realKey])
		}
	}
	return result
}

func (m *DeferredDeletionMetaStore) List() []*ObjectWrapper {
	m.lock.RLock()
	defer m.lock.RUnlock()
	result := make([]*ObjectWrapper, 0)
	for _, item := range m.Items {
		result = append(result, item)
	}
	return result
}

func (m *DeferredDeletionMetaStore) handleEvent() {
	defer panicRecover()
	for {
		select {
		case event := <-m.eventCh:
			switch event.EventType {
			case EventTypeAdd:
				m.handleAddEvent(event)
			case EventTypeUpdate:
				m.handleUpdateEvent(event)
			case EventTypeDelete:
				m.handleDeleteEvent(event)
			case EventTypeDeferredDelete:
				m.handleDeferredDeleteEvent(event)
			default:
				logger.Error(context.Background(), "unknown event type", event.EventType)
			}
		case <-m.stopCh:
			return
		}
	}
}

func (m *DeferredDeletionMetaStore) handleAddEvent(event *K8sMetaEvent) {
	m.handleAddOrUpdateEvent(event)
}

func (m *DeferredDeletionMetaStore) handleUpdateEvent(event *K8sMetaEvent) {
	m.handleAddOrUpdateEvent(event)
}

func (m *DeferredDeletionMetaStore) handleAddOrUpdateEvent(event *K8sMetaEvent) {
	key, err := m.keyFunc(event.Object.Raw)
	if err != nil {
		logger.Error(context.Background(), "K8S_META_HANDLE_ALARM", "handle k8s meta with keyFunc error", err)
		return
	}
	idxKeys := m.getIdxKeys(event.Object)
	m.lock.Lock()
	defer m.lock.Unlock()
	m.Items[key] = event.Object
	for _, idxKey := range idxKeys {
		if _, ok := m.Index[idxKey]; !ok {
			m.Index[idxKey] = make([]string, 0)
		}
		m.Index[idxKey] = append(m.Index[idxKey], key)
	}
}

func (m *DeferredDeletionMetaStore) handleDeleteEvent(event *K8sMetaEvent) {
	go func() {
		// wait and add a deferred delete event
		time.Sleep(time.Duration(m.gracePeriod) * time.Second)
		event.Object.Deleted = true
		m.eventCh <- &K8sMetaEvent{
			EventType: EventTypeDeferredDelete,
			Object:    event.Object,
		}
	}()
}

func (m *DeferredDeletionMetaStore) handleDeferredDeleteEvent(event *K8sMetaEvent) {
	key, err := m.keyFunc(event.Object.Raw)
	if err != nil {
		logger.Error(context.Background(), "handleDeferredDeleteEvent keyFunc error", err)
		return
	}
	idxKeys := m.getIdxKeys(event.Object)
	m.lock.Lock()
	defer m.lock.Unlock()
	if obj, ok := m.Items[key]; ok {
		if obj.Deleted {
			delete(m.Items, key)
			for _, idxKey := range idxKeys {
				for i, k := range m.Index[idxKey] {
					if k == key {
						m.Index[idxKey] = append(m.Index[idxKey][:i], m.Index[idxKey][i+1:]...)
						break
					}
				}
				if len(m.Index[idxKey]) == 0 {
					delete(m.Index, idxKey)
				}
			}
		} else {
			// there is a new add event between delete event and deferred delete event
			// clear invalid index
			newIdxKeys := m.getIdxKeys(obj)
			for i := range idxKeys {
				if idxKeys[i] != newIdxKeys[i] {
					for j, k := range m.Index[idxKeys[i]] {
						if k == key {
							m.Index[idxKeys[i]] = append(m.Index[idxKeys[i]][:j], m.Index[idxKeys[i]][j+1:]...)
							break
						}
					}
					if len(m.Index[idxKeys[i]]) == 0 {
						delete(m.Index, idxKeys[i])
					}
				}
			}
		}
	}
}

func (m *DeferredDeletionMetaStore) getIdxKeys(obj *ObjectWrapper) []string {
	result := make([]string, 0)
	for _, rule := range m.indexRules {
		idxKeys, err := rule(obj.Raw)
		if err != nil {
			logger.Error(context.Background(), "K8S_META_HANDLE_ALARM", "handle k8s meta with idx rules error", err)
			return nil
		}
		result = append(result, idxKeys...)
	}
	return result
}
