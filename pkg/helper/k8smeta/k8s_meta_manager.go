package k8smeta

import (
	"context"
	"fmt"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/rest"
	"k8s.io/client-go/tools/clientcmd"
	controllerConfig "sigs.k8s.io/controller-runtime/pkg/client/config"

	"github.com/alibaba/ilogtail/pkg/logger"
)

var metaManager *MetaManager

var onceManager sync.Once

type MetaCache interface {
	Get(key []string) map[string][]*ObjectWrapper
	List() []*ObjectWrapper
	RegisterSendFunc(key string, sendFunc SendFunc, interval int)
	UnRegisterSendFunc(key string)
	init(*kubernetes.Clientset)
	watch(stopCh <-chan struct{})
}

type FlushCh struct {
	Ch         chan *K8sMetaEvent
	ConfigName string
}

type MetaManager struct {
	clientset *kubernetes.Clientset
	stopCh    chan struct{}

	ready atomic.Bool

	metadataHandler  *metadataHandler
	cacheMap         map[string]MetaCache
	linkGenerator    *LinkGenerator
	linkRegisterMap  map[string][]string
	linkRegisterLock sync.RWMutex

	// self metrics
	addEventCount      atomic.Int64
	updateEventCount   atomic.Int64
	deleteEventCount   atomic.Int64
	cacheResourceCount atomic.Int64
}

func GetMetaManagerInstance() *MetaManager {
	onceManager.Do(func() {
		metaManager = &MetaManager{
			stopCh: make(chan struct{}),
		}
		metaManager.metadataHandler = newMetadataHandler(metaManager)
		metaManager.cacheMap = make(map[string]MetaCache)
		for _, resource := range AllResources {
			metaManager.cacheMap[resource] = newK8sMetaCache(metaManager.stopCh, resource)
		}
		metaManager.linkGenerator = NewK8sMetaLinkGenerator(metaManager.cacheMap)
		metaManager.linkRegisterMap = make(map[string][]string)
	})
	return metaManager
}

func (m *MetaManager) Init(configPath string) (err error) {
	var config *rest.Config
	if len(configPath) > 0 {
		config, err = clientcmd.BuildConfigFromFlags("", configPath)
		if err != nil {
			return err
		}
	} else {
		// 创建 Kubernetes 客户端配置
		config = controllerConfig.GetConfigOrDie()
	}
	// 创建 Kubernetes 客户端
	clientset, err := kubernetes.NewForConfig(config)
	if err != nil {
		return err
	}
	m.clientset = clientset

	go func() {
		startTime := time.Now()
		for _, cache := range m.cacheMap {
			cache.init(clientset)
		}
		m.ready.Store(true)
		logger.Info(context.Background(), "init k8s meta manager", "success", "latancy (ms)", fmt.Sprintf("%d", time.Since(startTime).Milliseconds()))
	}()
	return nil
}

func (m *MetaManager) Run(stopCh chan struct{}) {
	m.stopCh = stopCh
	m.runServer()
}

func (m *MetaManager) IsReady() bool {
	return m.ready.Load()
}

func (m *MetaManager) RegisterSendFunc(configName string, resourceType string, sendFunc SendFunc, interval int) {
	if cache, ok := m.cacheMap[resourceType]; ok {
		cache.RegisterSendFunc(configName, func(events []*K8sMetaEvent) {
			sendFunc(events)
			linkTypeList := make([]string, 0)
			m.linkRegisterLock.RLock()
			if m.linkRegisterMap[configName] != nil {
				linkTypeList = append(linkTypeList, m.linkRegisterMap[configName]...)
			}
			m.linkRegisterLock.RUnlock()
			for _, linkType := range linkTypeList {
				linkEvents := m.linkGenerator.GenerateLinks(events, linkType)
				if linkEvents != nil {
					sendFunc(linkEvents)
				}
			}
		}, interval)
		return
	}
	if !isEntity(resourceType) {
		m.linkRegisterLock.Lock()
		if _, ok := m.linkRegisterMap[configName]; !ok {
			m.linkRegisterMap[configName] = make([]string, 0)
		}
		m.linkRegisterMap[configName] = append(m.linkRegisterMap[configName], resourceType)
		m.linkRegisterLock.Unlock()
	} else {
		logger.Error(context.Background(), "ENTITY_PIPELINE_REGISTER_ERROR", "resourceType not support", resourceType)
	}
}

func (m *MetaManager) UnRegisterSendFunc(configName string, resourceType string) {
	if cache, ok := m.cacheMap[resourceType]; ok {
		cache.UnRegisterSendFunc(configName)
	} else {
		logger.Error(context.Background(), "ENTITY_PIPELINE_UNREGISTER_ERROR", "resourceType not support", resourceType)
	}
}

func GetMetaManagerMetrics() []map[string]string {
	manager := GetMetaManagerInstance()
	if manager == nil || !manager.IsReady() {
		return nil
	}
	metrics := make([]map[string]string, 0)
	// cache
	queueLen := 0
	for _, cache := range manager.cacheMap {
		queueLen += len(cache.(*k8sMetaCache).eventCh)
	}
	metrics = append(metrics, map[string]string{
		"value.k8s_meta_add_event_count":       fmt.Sprintf("%d", manager.addEventCount.Load()),
		"value.k8s_meta_update_event_count":    fmt.Sprintf("%d", manager.updateEventCount.Load()),
		"value.k8s_meta_delete_event_count":    fmt.Sprintf("%d", manager.deleteEventCount.Load()),
		"value.k8s_meta_cache_resource_count":  fmt.Sprintf("%d", manager.cacheResourceCount.Load()),
		"value.k8s_meta_event_queue_len_total": fmt.Sprintf("%d", queueLen),
	})
	manager.addEventCount.Add(-manager.addEventCount.Load())
	manager.updateEventCount.Add(-manager.updateEventCount.Load())
	manager.deleteEventCount.Add(-manager.deleteEventCount.Load())

	// http server
	httpServerMetrics := manager.metadataHandler.GetMetrics()
	metrics = append(metrics, httpServerMetrics)
	return metrics
}

func (m *MetaManager) AddEventCount() {
	m.addEventCount.Add(1)
	m.cacheResourceCount.Add(1)
}

func (m *MetaManager) UpdateEventCount() {
	m.updateEventCount.Add(1)
}

func (m *MetaManager) DeleteEventCount() {
	m.deleteEventCount.Add(1)
	m.cacheResourceCount.Add(-1)
}

func (m *MetaManager) runServer() {
	go m.metadataHandler.K8sServerRun(m.stopCh)
}

func isEntity(resourceType string) bool {
	return !strings.Contains(resourceType, LINK_SPLIT_CHARACTER)
}
