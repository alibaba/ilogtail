package k8smeta

import (
	"sync"
	"time"

	"k8s.io/client-go/tools/cache"
)

type HandlerFunc func(obj *ObjectWrapper)

type cacheMetaStore struct {
	addHandlers map[string]HandlerFunc
	addLock     sync.RWMutex

	updateHandlers map[string]HandlerFunc
	updateLock     sync.RWMutex

	deleteHandlers map[string]HandlerFunc
	deleteLock     sync.RWMutex

	keyFunc cache.KeyFunc

	Items map[string]*ObjectWrapper
	lock  sync.RWMutex
}

func (m *cacheMetaStore) RegisterHandlers(addHandler, updateHandler, deleteHandler HandlerFunc, configName string) {
	if addHandler != nil {
		m.addLock.Lock()
		m.addHandlers[configName] = addHandler
		m.addLock.Unlock()
	}

	if updateHandler != nil {
		m.updateLock.Lock()
		m.updateHandlers[configName] = updateHandler
		m.updateLock.Unlock()
	}

	if deleteHandler != nil {
		m.deleteLock.Lock()
		m.deleteHandlers[configName] = deleteHandler
		m.deleteLock.Unlock()
	}
}

func (m *cacheMetaStore) UnregisterHandlers(configName string) {
	m.addLock.Lock()
	delete(m.addHandlers, configName)
	m.addLock.Unlock()

	m.updateLock.Lock()
	delete(m.updateHandlers, configName)
	m.updateLock.Unlock()

	m.deleteLock.Lock()
	delete(m.deleteHandlers, configName)
	m.deleteLock.Unlock()
}

// 不cache，只处理
func (m *cacheMetaStore) AddWithoutCache(obj *ObjectWrapper) error {
	m.addLock.RLock()
	defer m.addLock.RUnlock()
	for _, handler := range m.addHandlers {
		handler(obj)
	}
	return nil
}

func (m *cacheMetaStore) Add(obj interface{}) error {
	key, err := m.keyFunc(obj)
	if err != nil {
		return err
	}
	m.lock.Lock()
	nowTime := time.Now().Unix()
	objectWrapper := &ObjectWrapper{
		RawObject:  obj,
		IsDeleted:  false,
		CreateTime: nowTime,
		UpdateTime: nowTime,
	}

	m.Items[key] = objectWrapper
	m.lock.Unlock()

	m.addLock.RLock()
	defer m.addLock.RUnlock()
	for _, handler := range m.addHandlers {
		handler(objectWrapper)
	}
	return nil
}

func (m *cacheMetaStore) Update(obj interface{}) error {
	key, err := m.keyFunc(obj)
	if err != nil {
		return err
	}
	m.lock.Lock()
	objWrapper, ok := m.Items[key]
	nowTime := time.Now().Unix()
	if ok {
		objWrapper.UpdateTime = nowTime
		objWrapper.RawObject = obj
		objWrapper.IsDeleted = false
	} else {
		objWrapper = &ObjectWrapper{
			RawObject:  obj,
			IsDeleted:  false,
			CreateTime: nowTime,
			UpdateTime: nowTime,
		}
		m.Items[key] = objWrapper
	}
	m.lock.Unlock()

	m.updateLock.RLock()
	defer m.updateLock.RUnlock()
	for _, handler := range m.updateHandlers {
		handler(objWrapper)
	}
	return nil
}

func (m *cacheMetaStore) Delete(obj interface{}) error {
	key, err := m.keyFunc(obj)
	if err != nil {
		return err
	}

	m.lock.Lock()
	objWrapper, ok := m.Items[key]
	nowTime := time.Now().Unix()
	if !ok {
		// 找不到就已经删了
		return nil
	}
	if !objWrapper.IsDeleted {
		objWrapper.UpdateTime = nowTime
		objWrapper.IsDeleted = true
	}
	objWrapper.RawObject = obj

	delete(m.Items, key)
	m.lock.Unlock()

	m.deleteLock.RLock()
	defer m.deleteLock.RUnlock()
	for _, handler := range m.deleteHandlers {
		handler(objWrapper)
	}
	return nil
}

// List implements the List method of the store interface.
func (m *cacheMetaStore) List() []interface{} {
	m.lock.RLock()
	defer m.lock.RUnlock()

	list := make([]interface{}, 0, len(m.Items))
	for _, item := range m.Items {
		if !item.IsDeleted {
			list = append(list, item.RawObject)
		}
	}
	return list
}

// custom implement
func (m *cacheMetaStore) ListObjWrapper() []*ObjectWrapper {
	m.lock.RLock()
	defer m.lock.RUnlock()

	list := make([]*ObjectWrapper, 0, len(m.Items))
	for _, item := range m.Items {
		if !item.IsDeleted {
			list = append(list, item)
		}
	}
	return list
}

func (m *cacheMetaStore) ListObjWrapperByKeys(keys []string) map[string]*ObjectWrapper {
	m.lock.RLock()
	defer m.lock.RUnlock()

	res := make(map[string]*ObjectWrapper)
	for _, key := range keys {
		if len(key) == 0 {
			continue
		}
		if obj, ok := m.Items[key]; ok {
			res[key] = obj
		}
	}
	return res
}

// ListKeys implements the ListKeys method of the store interface.
func (m *cacheMetaStore) ListKeys() []string {
	return nil
}

// Get implements the Get method of the store interface.
func (m *cacheMetaStore) Get(obj interface{}) (item interface{}, exists bool, err error) {
	return nil, false, nil
}

// GetByKey implements the GetByKey method of the store interface.
func (m *cacheMetaStore) GetByKey(key string) (item interface{}, exists bool, err error) {
	return nil, false, nil
}

func (m *cacheMetaStore) Replace(list []interface{}, _ string) error {
	for _, o := range list {
		err := m.Add(o)
		if err != nil {
			return err
		}
	}
	return nil
}

func (m *cacheMetaStore) Resync() error {
	return nil
}
