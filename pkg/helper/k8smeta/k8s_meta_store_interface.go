package k8smeta

import "k8s.io/client-go/tools/cache"

type ObjectWrapper struct {
	IsDeleted  bool
	RawObject  interface{}
	CreateTime int64
	UpdateTime int64
}


type K8sMetaStore interface {
	cache.Store
	AddWithoutCache(obj *ObjectWrapper) error
	ListObjWrapper() []*ObjectWrapper

	RegisterHandlers(addHandler, updateHandler, deleteHandler HandlerFunc, configName string)
	UnregisterHandlers(configName string)
	ListObjWrapperByKeys(keys []string) map[string]*ObjectWrapper
}
