package k8smeta

import (
	"context"
	"sync"
	"sync/atomic"

	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/rest"
	"k8s.io/client-go/tools/clientcmd"

	"github.com/alibaba/ilogtail/pkg/logger"
)

var metaManager *MetaManager

var onceManager sync.Once

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
		config, err = rest.InClusterConfig()
		if err != nil {
			return err
		}
	}
	// 创建 Kubernetes 客户端
	clientset, err := kubernetes.NewForConfig(config)
	if err != nil {
		return err
	}
	m.clientset = clientset

	go func() {
		m.ServiceProcessor = &serviceProcessor{
			metaManager: m,
		}
		m.ServiceProcessor.init(m.stopCh, m.eventCh)
		m.PodProcessor = &podProcessor{
			metaManager:      m,
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
		for {
			select {
			case event := <-m.eventCh:
				m.pipelineChsLock.RLock()
				pipelineChs := m.pipelineChs[event.ResourceType]
				for _, pipelineCh := range pipelineChs {
					select {
					case pipelineCh.Ch <- event:
					default:
						logger.Error(context.Background(), "ENTITY_PIPELINE_QUEUE_FULL", "pipelineCh is full, discard in current sync")
					}
				}
				m.pipelineChsLock.RUnlock()
			case <-m.stopCh:
				return
			}
		}
	}()
}
