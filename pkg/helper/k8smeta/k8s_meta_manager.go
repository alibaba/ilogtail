package k8smeta

import (
	"sync"

	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/rest"
	"k8s.io/client-go/tools/clientcmd"
)

var metaManager *MetaManager

var onceManager sync.Once

var refreshInterval = 60

type MetaManager struct {
	clientset *kubernetes.Clientset

	// 延迟删除的
	PodProcessor *podProcessor
	//
	ServiceProcessor *serviceProcessor
}

type MetaProcessor interface {
	init()
	watch(stopCh <-chan struct{})
	RegisterHandlers(addHandler, updateHandler, deleteHandler HandlerFunc, configName string)
	UnregisterHandlers(configName string)
}

func GetMetaManagerInstance() *MetaManager {
	onceManager.Do(func() {
		metaManager = &MetaManager{}
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

	m.ServiceProcessor = &serviceProcessor{
		metaManager: m,
	}
	m.ServiceProcessor.init()

	m.PodProcessor = &podProcessor{
		metaManager:      m,
		serviceMetaStore: m.ServiceProcessor.metaStore,
	}
	m.PodProcessor.init()

	return nil
}

func (m *MetaManager) Run(stopCh <-chan struct{}) {

	m.ServiceProcessor.watch(stopCh)
	m.PodProcessor.watch(stopCh)

	// 最后
	m.runServer(stopCh)
}

func (m *MetaManager) runServer(stopCh <-chan struct{}) {
	// meta server需要注册的handler
	metadataHandler := NewMetadataHandler(m.PodProcessor.metaStore)
	m.PodProcessor.metaStore.DeferredDeleteHandler = metadataHandler.watchCache.handlePodDelete
	go metadataHandler.K8sServerRun(stopCh)
}
