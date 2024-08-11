package k8smeta

import (
	"context"
	"sync"
	"sync/atomic"
	"time"

	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/rest"
	"k8s.io/client-go/tools/clientcmd"
	controllerConfig "sigs.k8s.io/controller-runtime/pkg/client/config"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/util"
)

var metaManager *MetaManager

var onceManager sync.Once

type MetaProcessor interface {
	Get(key []string) map[string][]*ObjectWrapper
	init(stopCh chan struct{}, pipelineCh chan *K8sMetaEvent)
	watch(stopCh <-chan struct{})
	flushRealTimeEvent(event *K8sMetaEvent)
	flushPeriodEvent(events []*K8sMetaEvent)
}

type FlushCh struct {
	Ch         chan *K8sMetaEvent
	ConfigName string
}

type MetaManager struct {
	clientset *kubernetes.Clientset
	stopCh    chan struct{}

	eventCh         chan *K8sMetaEvent
	pipelineChs     map[string][]*FlushCh
	pipelineChsLock sync.RWMutex
	ready           atomic.Bool

	PodProcessor     *podProcessor
	ServiceProcessor *serviceProcessor
}

func GetMetaManagerInstance() *MetaManager {
	onceManager.Do(func() {
		metaManager = &MetaManager{
			stopCh:      make(chan struct{}),
			eventCh:     make(chan *K8sMetaEvent, 100),
			pipelineChs: make(map[string][]*FlushCh),
		}
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
		m.ServiceProcessor = &serviceProcessor{
			clientset: m.clientset,
		}
		m.ServiceProcessor.init(m.stopCh, m.eventCh)
		m.PodProcessor = &podProcessor{
			clientset:        m.clientset,
			serviceMetaStore: m.ServiceProcessor.metaStore,
		}
		m.PodProcessor.init(m.stopCh, m.eventCh)
		m.ready.Store(true)
		logger.Info(context.Background(), "init k8s meta manager", "success")
	}()
	return nil
}

func (m *MetaManager) Run(stopCh chan struct{}) {
	m.stopCh = stopCh
	m.runServer()
	m.runFlush()
}

func (m *MetaManager) IsReady() bool {
	return m.ready.Load()
}

func (m *MetaManager) List(resourceTypes []string) []*ObjectWrapper {
	objs := make([]*ObjectWrapper, 0)
	for _, resourceType := range resourceTypes {
		switch resourceType {
		case POD:
			podServiceLink := util.Contains(resourceTypes, POD_SERVICE)
			objs = append(objs, m.PodProcessor.List(podServiceLink)...)
		case SERVICE:
			objs = append(objs, m.ServiceProcessor.List()...)
		}
	}
	return objs
}

func (m *MetaManager) RegisterFlush(ch chan *K8sMetaEvent, configName string, resourceType string) {
	m.pipelineChsLock.Lock()
	defer m.pipelineChsLock.Unlock()
	if _, ok := m.pipelineChs[resourceType]; !ok {
		m.pipelineChs[resourceType] = make([]*FlushCh, 0)
	}
	pipelineCh := &FlushCh{
		Ch:         ch,
		ConfigName: configName,
	}
	m.pipelineChs[resourceType] = append(m.pipelineChs[resourceType], pipelineCh)
}

func (m *MetaManager) UnRegisterFlush(configName string, resourceType string) {
	m.pipelineChsLock.Lock()
	defer m.pipelineChsLock.Unlock()
	if _, ok := m.pipelineChs[resourceType]; ok {
		for i, pipelineCh := range m.pipelineChs[resourceType] {
			if pipelineCh.ConfigName == configName {
				m.pipelineChs[resourceType] = append(m.pipelineChs[resourceType][:i], m.pipelineChs[resourceType][i+1:]...)
			}
		}
	}
}

func (m *MetaManager) runServer() {
	metadataHandler := newMetadataHandler()
	go metadataHandler.K8sServerRun(m.stopCh)
}

func (m *MetaManager) runFlush() {
	go func() {
		flushBlockTimes := 0
		lastBlockTime := time.Now()
		for {
			select {
			case event := <-m.eventCh:
				m.pipelineChsLock.RLock()
				pipelineChs := m.pipelineChs[event.Object.ResourceType]
				for _, pipelineCh := range pipelineChs {
					select {
					case pipelineCh.Ch <- event:
					default:
						if time.Since(lastBlockTime) > time.Second*3 || flushBlockTimes > 100 {
							logger.Error(context.Background(), "ENTITY_PIPELINE_QUEUE_FULL", "pipelineCh is full, discard in current sync, count:", flushBlockTimes)
							flushBlockTimes = 0
							lastBlockTime = time.Now()
						}
					}
				}
				m.pipelineChsLock.RUnlock()
			case <-m.stopCh:
				return
			}
		}
	}()
}
