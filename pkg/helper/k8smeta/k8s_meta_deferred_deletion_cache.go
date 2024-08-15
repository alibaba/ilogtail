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

	// timer
	gracePeriod int64
	sendFuncs   sync.Map
}

type SendFuncWithStopCh struct {
	SendFunc SendFunc
	StopCh   chan struct{}
}

func NewDeferredDeletionMetaStore(eventCh chan *K8sMetaEvent, stopCh <-chan struct{}, gracePeriod int64, keyFunc cache.KeyFunc, indexRules ...IdxFunc) *DeferredDeletionMetaStore {
	m := &DeferredDeletionMetaStore{
		keyFunc:    keyFunc,
		indexRules: indexRules,

		eventCh: eventCh,
		stopCh:  stopCh,

		Items: make(map[string]*ObjectWrapper),
		Index: make(map[string][]string),

		gracePeriod: gracePeriod,
		sendFuncs:   sync.Map{},
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

func (m *DeferredDeletionMetaStore) RegisterSendFunc(key string, f SendFunc, interval int) {
	sendFuncWithStopCh := &SendFuncWithStopCh{
		SendFunc: f,
		StopCh:   make(chan struct{}),
	}
	m.sendFuncs.Store(key, sendFuncWithStopCh)
	go func() {
		for {
			snapshotOfKeys := make([]string, 0)
			m.lock.RLock()
			for k := range m.Items {
				snapshotOfKeys = append(snapshotOfKeys, k)
			}
			m.lock.RUnlock()
			if len(snapshotOfKeys) == 0 {
				select {
				case <-time.After(time.Second * time.Duration(interval)):
				case <-sendFuncWithStopCh.StopCh:
				}
				continue
			}
			eventInterval := time.Duration(int64(time.Second*time.Duration(interval)) / int64(len(snapshotOfKeys)))
			ticker := time.NewTicker(eventInterval)
			curIdx := 0
			done := false
			for !done {
				select {
				case <-ticker.C:
					if curIdx < len(snapshotOfKeys) {
						m.lock.RLock()
						if obj, ok := m.Items[snapshotOfKeys[curIdx]]; ok {
							obj.LastObservedTime = time.Now().Unix()
							f(&K8sMetaEvent{
								EventType: EventTypeUpdate,
								Object:    obj,
							})
							curIdx++
						}
						m.lock.RUnlock()
					} else {
						done = true
					}
				case <-sendFuncWithStopCh.StopCh:
					return
				}
			}
		}
	}()
}

func (m *DeferredDeletionMetaStore) UnRegisterSendFunc(key string) {
	if stopCh, ok := m.sendFuncs.LoadAndDelete(key); ok {
		close(stopCh.(chan struct{}))
	}
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
			m.sendFuncs.Range(func(key, value interface{}) bool {
				close(value.(chan struct{}))
				return true
			})
			return
		}
	}
}

func (m *DeferredDeletionMetaStore) handleAddEvent(event *K8sMetaEvent) {
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
	m.sendFuncs.Range(func(key, value interface{}) bool {
		value.(*SendFuncWithStopCh).SendFunc(event)
		return true
	})
}

func (m *DeferredDeletionMetaStore) handleUpdateEvent(event *K8sMetaEvent) {
	key, err := m.keyFunc(event.Object.Raw)
	if err != nil {
		logger.Error(context.Background(), "K8S_META_HANDLE_ALARM", "handle k8s meta with keyFunc error", err)
		return
	}
	idxKeys := m.getIdxKeys(event.Object)
	m.lock.Lock()
	defer m.lock.Unlock()
	if obj, ok := m.Items[key]; ok {
		event.Object.FirstObservedTime = obj.FirstObservedTime
	}
	m.Items[key] = event.Object
	for _, idxKey := range idxKeys {
		if _, ok := m.Index[idxKey]; !ok {
			m.Index[idxKey] = make([]string, 0)
		}
		m.Index[idxKey] = append(m.Index[idxKey], key)
	}
	m.sendFuncs.Range(func(key, value interface{}) bool {
		value.(*SendFuncWithStopCh).SendFunc(event)
		return true
	})
}

func (m *DeferredDeletionMetaStore) handleDeleteEvent(event *K8sMetaEvent) {
	key, err := m.keyFunc(event.Object.Raw)
	if err != nil {
		logger.Error(context.Background(), "K8S_META_HANDLE_ALARM", "handle k8s meta with keyFunc error", err)
		return
	}
	m.lock.Lock()
	defer m.lock.Unlock()
	if obj, ok := m.Items[key]; ok {
		obj.Deleted = true
	}
	m.sendFuncs.Range(func(key, value interface{}) bool {
		value.(*SendFuncWithStopCh).SendFunc(event)
		return true
	})
	go func() {
		// wait and add a deferred delete event
		time.Sleep(time.Duration(m.gracePeriod) * time.Second)
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
