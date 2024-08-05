package k8smeta

import (
	"context"
	"sync"
	"time"

	"k8s.io/client-go/tools/cache"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type DeferredDeletionMetaStore struct {
	keyFunc       cache.KeyFunc
	indexRules    []IdxFunc
	timerHandlers []TimerHandler

	eventCh chan *K8sMetaEvent
	stopCh  <-chan struct{}

	// cache
	Items map[string]*K8sMetaEvent
	Index map[string][]string
	lock  sync.RWMutex

	refreshInterval int64
	gracePeriod     int64
}

func NewDeferredDeletionMetaStore(eventCh chan *K8sMetaEvent, stopCh <-chan struct{}, refreshInterval, gracePeriod int64, keyFunc cache.KeyFunc, indexRules ...IdxFunc) *DeferredDeletionMetaStore {
	m := &DeferredDeletionMetaStore{
		keyFunc:       keyFunc,
		indexRules:    indexRules,
		timerHandlers: make([]TimerHandler, 0),

		eventCh: eventCh,
		stopCh:  stopCh,

		Items: make(map[string]*K8sMetaEvent),
		Index: make(map[string][]string),

		refreshInterval: refreshInterval,
		gracePeriod:     gracePeriod,
	}
	return m
}

func (m *DeferredDeletionMetaStore) Start(timerHandlers ...TimerHandler) {
	m.timerHandlers = timerHandlers
	go m.handleEvent()
}

func (m *DeferredDeletionMetaStore) Get(key []string) map[string][]*K8sMetaEvent {
	m.lock.RLock()
	defer m.lock.RUnlock()
	result := make(map[string][]*K8sMetaEvent)
	for _, k := range key {
		if realKeys, ok := m.Index[k]; !ok {
			return nil
		} else {
			for _, realKey := range realKeys {
				result[k] = append(result[k], m.Items[realKey])
			}
		}
	}
	return result
}

func (m *DeferredDeletionMetaStore) List() []*K8sMetaEvent {
	m.lock.RLock()
	defer m.lock.RUnlock()
	result := make([]*K8sMetaEvent, 0)
	for _, item := range m.Items {
		result = append(result, item)
	}
	return result
}

func (m *DeferredDeletionMetaStore) handleEvent() {
	ticker := time.NewTicker(time.Duration(m.refreshInterval) * time.Second)
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
		case <-ticker.C:
			m.handleTimerEvent()
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
	key, err := m.keyFunc(event.RawObject)
	if err != nil {
		logger.Error(context.Background(), "K8S_META_HANDLE_ALARM", "handle k8s meta with keyFunc error", err)
		return
	}
	idxKeys := m.getIdxKeys(event)
	m.lock.Lock()
	defer m.lock.Unlock()
	m.Items[key] = event
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
		m.eventCh <- &K8sMetaEvent{
			EventType:    EventTypeDeferredDelete,
			ResourceType: event.ResourceType,
			RawObject:    event.RawObject,
		}
	}()
}

func (m *DeferredDeletionMetaStore) handleDeferredDeleteEvent(event *K8sMetaEvent) {
	key, err := m.keyFunc(event.RawObject)
	if err != nil {
		logger.Error(context.Background(), "handleDeferredDeleteEvent keyFunc error", err)
		return
	}
	idxKeys := m.getIdxKeys(event)
	m.lock.Lock()
	defer m.lock.Unlock()
	delete(m.Items, key)
	for _, idxKey := range idxKeys {
		for i, k := range m.Index[idxKey] {
			if k == key {
				m.Index[idxKey] = append(m.Index[idxKey][:i], m.Index[idxKey][i+1:]...)
				break
			}
		}
	}
}

func (m *DeferredDeletionMetaStore) handleTimerEvent() {
	// flush all
	m.lock.Lock()
	itemList := make([]*K8sMetaEvent, 0)
	for _, item := range m.Items {
		itemList = append(itemList, item)
	}
	m.lock.Unlock()
	for _, handler := range m.timerHandlers {
		handler(itemList)
	}
}

func (m *DeferredDeletionMetaStore) getIdxKeys(event *K8sMetaEvent) []string {
	result := make([]string, 0)
	for _, rule := range m.indexRules {
		idxKeys, err := rule(event.RawObject)
		if err != nil {
			logger.Error(context.Background(), "K8S_META_HANDLE_ALARM", "handle k8s meta with idx rules error", err)
			return nil
		}
		result = append(result, idxKeys...)
	}
	return result
}
