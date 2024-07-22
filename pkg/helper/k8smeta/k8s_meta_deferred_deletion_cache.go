package k8smeta

import (
	"sync"
	"time"
)

type DeferredDeletionMetaStore struct {
	cacheMetaStore

	deleteKeysMutex sync.Mutex
	deleteKeys      map[string]struct{}

	DeferredDeleteHandler HandlerFunc

	deleteOnce sync.Once

	refreshInterval int64
	gracePeriod     int64
}

func (m *DeferredDeletionMetaStore) RegisterHandlers(addHandler, updateHandler, deleteHandler HandlerFunc, configName string) {
	m.cacheMetaStore.RegisterHandlers(addHandler, updateHandler, deleteHandler, configName)
	m.deleteLoop()
}

func (m *DeferredDeletionMetaStore) UnregisterHandlers(configName string) {
	m.cacheMetaStore.UnregisterHandlers(configName)
}

// 不cache，只处理
func (m *DeferredDeletionMetaStore) AddWithoutCache(obj *ObjectWrapper) error {
	return m.cacheMetaStore.AddWithoutCache(obj)
}

func (m *DeferredDeletionMetaStore) Add(obj interface{}) error {
	return m.cacheMetaStore.Add(obj)
}

func (m *DeferredDeletionMetaStore) Update(obj interface{}) error {
	return m.cacheMetaStore.Update(obj)
}

func (m *DeferredDeletionMetaStore) Delete(obj interface{}) error {
	key, err := m.keyFunc(obj)
	if err != nil {
		return err
	}
	{
		m.deleteKeysMutex.Lock()
		defer m.deleteKeysMutex.Unlock()
		m.deleteKeys[key] = struct{}{}
	}

	m.cacheMetaStore.lock.Lock()
	objWrapper, ok := m.cacheMetaStore.Items[key]
	nowTime := time.Now().Unix()
	if !ok {
		return nil
	}

	//	标记删除
	if !objWrapper.IsDeleted {
		objWrapper.UpdateTime = nowTime
		objWrapper.IsDeleted = true
	}
	objWrapper.RawObject = obj
	m.cacheMetaStore.lock.Unlock()

	m.cacheMetaStore.deleteLock.Lock()
	defer m.cacheMetaStore.deleteLock.Unlock()
	for _, handler := range m.cacheMetaStore.deleteHandlers {
		handler(objWrapper)
	}
	return nil
}

// List implements the List method of the store interface.
func (m *DeferredDeletionMetaStore) List() []interface{} {
	return m.cacheMetaStore.List()
}

// custom implement
func (m *DeferredDeletionMetaStore) ListObjWrapper() []*ObjectWrapper {
	return m.cacheMetaStore.ListObjWrapper()
}

func (m *DeferredDeletionMetaStore) ListObjWrapperByKeys(keys []string) map[string]*ObjectWrapper {
	return m.cacheMetaStore.ListObjWrapperByKeys(keys)
}

// ListKeys implements the ListKeys method of the store interface.
func (m *DeferredDeletionMetaStore) ListKeys() []string {
	return nil
}

// Get implements the Get method of the store interface.
func (m *DeferredDeletionMetaStore) Get(obj interface{}) (item interface{}, exists bool, err error) {
	return nil, false, nil
}

// GetByKey implements the GetByKey method of the store interface.
func (m *DeferredDeletionMetaStore) GetByKey(key string) (item interface{}, exists bool, err error) {
	return nil, false, nil
}

func (m *DeferredDeletionMetaStore) Replace(list []interface{}, str string) error {
	return m.cacheMetaStore.Replace(list, str)
}

func (m *DeferredDeletionMetaStore) Resync() error {
	return nil
}

func (m *DeferredDeletionMetaStore) deleteLoop() {
	m.deleteOnce.Do(func() {
		go func() {
			interval := time.Second * time.Duration(m.refreshInterval)
			ticker := time.NewTicker(interval)
			for range ticker.C {
				now := time.Now().Unix()
				m.deleteKeysMutex.Lock()
				m.lock.Lock()
				for key, _ := range m.deleteKeys {
					obj, ok := m.Items[key]
					if !ok || !obj.IsDeleted {
						// 如果没有找到，就从删除队列中删掉
						delete(m.deleteKeys, key)
						continue
					}
					// 找到了，如果超时就删除
					if now-obj.UpdateTime > m.gracePeriod {
						m.DeferredDeleteHandler(obj)
						delete(m.deleteKeys, key)
						delete(m.Items, key)
					}
				}
				m.lock.Unlock()
				m.deleteKeysMutex.Unlock()
			}
		}()
	})
}
