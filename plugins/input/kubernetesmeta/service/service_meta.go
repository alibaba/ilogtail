package service

import (
	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

type ProcessFunc func(data *k8smeta.ObjectWrapper, method string) *protocol.Log

type ServiceK8sMeta struct {
	// entity switch
	Pod                   bool
	Node                  bool
	Service               bool
	Deployment            bool
	DaemonSet             bool
	StatefulSet           bool
	Configmap             bool
	Secret                bool
	Job                   bool
	CronJob               bool
	Namespace             bool
	PersistentVolume      bool
	PersistentVolumeClaim bool
	StorageClass          bool
	Ingress               bool
	// entity link switch
	PodReplicasetLink bool
	PodServiceLink    bool
	// other
	metaManager *k8smeta.MetaManager
	collector   pipeline.Collector
	configName  string
}

// Init called for init some system resources, like socket, mutex...
// return interval(ms) and error flag, if interval is 0, use default interval
func (s *ServiceK8sMeta) Init(context pipeline.Context) (int, error) {
	s.metaManager = k8smeta.GetMetaManagerInstance()
	s.configName = context.GetConfigName()

	return 0, nil
}

// Description returns a one-sentence description on the Input
func (s *ServiceK8sMeta) Description() string {
	return ""
}

// Stop stops the services and closes any necessary channels and connections
func (s *ServiceK8sMeta) Stop() error {
	if s.Pod {
		s.metaManager.PodProcessor.UnregisterHandlers(s.configName)
	}
	if s.Service {
		s.metaManager.ServiceProcessor.UnregisterHandlers(s.configName)
	}
	return nil
}

func (s *ServiceK8sMeta) Start(collector pipeline.Collector) error {
	s.collector = collector
	// 注册需要在collector之后做
	if s.Pod {
		podCollector := &podCollector{
			serviceK8sMeta: s,
			metaManager:    s.metaManager,
			collector:      s.collector,
		}
		podCollector.init()
	}

	if s.Service {
		serviceCollector := &serviceCollector{
			serviceK8sMeta: s,
			metaManager:    s.metaManager,
			collector:      s.collector,
		}
		serviceCollector.init()
	}
	return nil
}

func init() {
	pipeline.ServiceInputs["service_kubernetes_meta"] = func() pipeline.ServiceInput {
		return &ServiceK8sMeta{}
	}
}
