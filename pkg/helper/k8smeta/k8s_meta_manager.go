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

	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
)

var metaManager *MetaManager

var onceManager sync.Once

type MetaCache interface {
	Get(key []string) map[string][]*ObjectWrapper
	GetSize() int
	GetQueueSize() int
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
	metricRecord       pipeline.MetricsRecord
	addEventCount      pipeline.CounterMetric
	updateEventCount   pipeline.CounterMetric
	deleteEventCount   pipeline.CounterMetric
	cacheResourceGauge pipeline.GaugeMetric
	queueSizeGauge     pipeline.GaugeMetric
	httpRequestCount   pipeline.CounterMetric
	httpAvgDelayMs     pipeline.CounterMetric
	httpMaxDelayMs     pipeline.GaugeMetric
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

	m.metricRecord = pipeline.MetricsRecord{}
	m.addEventCount = helper.NewCounterMetricAndRegister(&m.metricRecord, helper.MetricComponentK8sMetaAddEventTotal)
	m.updateEventCount = helper.NewCounterMetricAndRegister(&m.metricRecord, helper.MetricComponentK8sMetaUpdateEventTotal)
	m.deleteEventCount = helper.NewCounterMetricAndRegister(&m.metricRecord, helper.MetricComponentK8sMetaDeleteEventTotal)
	m.cacheResourceGauge = helper.NewGaugeMetricAndRegister(&m.metricRecord, helper.MetricComponentK8sMetaCacheResourceSize)
	m.queueSizeGauge = helper.NewGaugeMetricAndRegister(&m.metricRecord, helper.MetricComponentK8sMetaQueueSize)
	m.httpRequestCount = helper.NewCounterMetricAndRegister(&m.metricRecord, helper.MetricComponentK8sMetaHTTPRequestTotal)
	m.httpAvgDelayMs = helper.NewAverageMetricAndRegister(&m.metricRecord, helper.MetricComponentK8sMetaHTTPAvgDelayMs)
	m.httpMaxDelayMs = helper.NewMaxMetricAndRegister(&m.metricRecord, helper.MetricComponentK8sMetaHTTPMaxDelayMs)

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
	// cache
	queueSize := 0
	cacheSize := 0
	for _, cache := range manager.cacheMap {
		queueSize += cache.GetQueueSize()
		cacheSize += cache.GetSize()

	}
	manager.queueSizeGauge.Set(float64(queueSize))
	manager.cacheResourceGauge.Set(float64(cacheSize))
	return []map[string]string{
		manager.metricRecord.ExportMetricRecords(),
	}
}

func (m *MetaManager) runServer() {
	go m.metadataHandler.K8sServerRun(m.stopCh)
}

func isEntity(resourceType string) bool {
	return !strings.Contains(resourceType, LINK_SPLIT_CHARACTER)
}
