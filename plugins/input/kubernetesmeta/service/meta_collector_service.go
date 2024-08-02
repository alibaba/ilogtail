package service

import (
	"strconv"
	"time"

	v1 "k8s.io/api/core/v1"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

type serviceCollector struct {
	serviceK8sMeta    *ServiceK8sMeta
	serviceProcessors []ProcessFunc
	metaManager       *k8smeta.MetaManager
	collector         pipeline.Collector

	serviceCh chan *k8smeta.K8sMetaEvent
}

func (s *serviceCollector) init() {
	s.registerInnerProcessor()
	if s.serviceK8sMeta.Service {
		s.serviceCh = make(chan *k8smeta.K8sMetaEvent, 100)
		s.metaManager.RegisterFlush(s.serviceCh, s.serviceK8sMeta.configName, k8smeta.SERVICE)
	}
	s.handleServiceEvent()
}

func (s *serviceCollector) handleServiceEvent() {
	go func() {
		for data := range s.serviceCh {
			switch data.EventType {
			case k8smeta.EventTypeAdd:
				s.HandleServiceAdd(data)
			case k8smeta.EventTypeDelete:
				s.HandleServiceDelete(data)
			default:
				s.HandleServiceUpdate(data)
			}
		}
	}()
}

func (s *serviceCollector) HandleServiceAdd(data *k8smeta.K8sMetaEvent) {
	for _, processor := range s.serviceProcessors {
		log := processor(data, "create")
		if log != nil {
			s.collector.AddRawLog(log)
		}
	}
}

func (s *serviceCollector) HandleServiceUpdate(data *k8smeta.K8sMetaEvent) {
	for _, processor := range s.serviceProcessors {
		log := processor(data, "update")
		if log != nil {
			s.collector.AddRawLog(log)
		}
	}
}

func (s *serviceCollector) HandleServiceDelete(data *k8smeta.K8sMetaEvent) {
	for _, processor := range s.serviceProcessors {
		log := processor(data, "delete")
		if log != nil {
			s.collector.AddRawLog(log)
		}
	}
}

func genKeyByService(service *v1.Service) string {
	return genKey(service.Namespace, service.Kind, service.Name)
}

func (s *serviceCollector) registerInnerProcessor() {
	if s.serviceK8sMeta.Service {
		s.serviceProcessors = append(s.serviceProcessors, s.processServiceEntity)
	}
}

func (s *serviceCollector) processServiceEntity(data *k8smeta.K8sMetaEvent, method string) *protocol.Log {
	log := &protocol.Log{}
	if obj, ok := data.RawObject.(*v1.Service); ok {
		log.Contents = make([]*protocol.Log_Content, 0)
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__domain__", Value: "k8s"})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__entity_type__", Value: "service"})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__entity_id__", Value: genKeyByService(obj)})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__method__", Value: method})

		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__first_observed_time__", Value: strconv.FormatInt(data.CreateTime, 10)})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__last_observed_time__", Value: strconv.FormatInt(data.UpdateTime, 10)})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__keep_alive_seconds__", Value: "3600"})

		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "namespace", Value: obj.Namespace})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "name", Value: obj.Name})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__category__", Value: "entity"})
		protocol.SetLogTime(log, uint32(time.Now().Unix()))
		return log
	}
	return nil
}
