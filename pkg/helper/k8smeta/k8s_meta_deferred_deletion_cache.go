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

type TimerEvent struct {
	ConfigName string
	Interval   int
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
		defer panicRecover()
		event := &K8sMetaEvent{
			EventType: EventTypeTimer,
			Object: &ObjectWrapper{
				Raw: &TimerEvent{
					ConfigName: key,
					Interval:   interval,
				},
			},
		}
		manager := GetMetaManagerInstance()
		for {
			if manager.IsReady() {
				break
			}
			time.Sleep(1 * time.Second)
		}

		m.eventCh <- event
		ticker := time.NewTicker(time.Duration(interval) * time.Second)
		for {
			select {
			case <-ticker.C:
				m.eventCh <- event
			case <-sendFuncWithStopCh.StopCh:
				return
			}
		}
	}()
}

func (m *DeferredDeletionMetaStore) UnRegisterSendFunc(key string) {
	if stopCh, ok := m.sendFuncs.LoadAndDelete(key); ok {
		close(stopCh.(*SendFuncWithStopCh).StopCh)
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
			case EventTypeTimer:
				m.handleTimerEvent(event)
			default:
				logger.Error(context.Background(), "unknown event type", event.EventType)
			}
		case <-m.stopCh:
			m.sendFuncs.Range(func(key, value interface{}) bool {
				close(value.(*SendFuncWithStopCh).StopCh)
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
	m.Items[key] = event.Object
	for _, idxKey := range idxKeys {
		if _, ok := m.Index[idxKey]; !ok {
			m.Index[idxKey] = make([]string, 0)
		}
		m.Index[idxKey] = append(m.Index[idxKey], key)
	}
	m.lock.Unlock()
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
	m.lock.Unlock()
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
	if obj, ok := m.Items[key]; ok {
		obj.Deleted = true
	}
	m.lock.Unlock()
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

func (m *DeferredDeletionMetaStore) handleTimerEvent(event *K8sMetaEvent) {
	timerEvent := event.Object.Raw.(*TimerEvent)
	if f, ok := m.sendFuncs.Load(timerEvent.ConfigName); ok {
		sendFuncWithStopCh := f.(*SendFuncWithStopCh)
		m.lock.RLock()
		defer m.lock.RUnlock()
		for _, obj := range m.Items {
			obj.LastObservedTime = time.Now().Unix()
			sendFuncWithStopCh.SendFunc(&K8sMetaEvent{
				EventType: EventTypeUpdate,
				Object:    obj,
			})
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
