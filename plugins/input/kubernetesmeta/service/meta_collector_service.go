package service

import (
	"strconv"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	v1 "k8s.io/api/core/v1"
)

type serviceCollector struct {
	serviceK8sMeta    *ServiceK8sMeta
	serviceProcessors []ProcessFunc
	metaManager       *k8smeta.MetaManager
	collector         pipeline.Collector
}

func (p *serviceCollector) init() {
	p.registerInnerProcessor()
	if p.serviceK8sMeta.Service {
		p.metaManager.ServiceProcessor.RegisterHandlers(p.HandleServiceAdd, p.HandleServiceUpdate, p.HandleServiceDelete, p.serviceK8sMeta.configName)
	}
}

func (s *serviceCollector) HandleServiceAdd(data *k8smeta.ObjectWrapper) {
	for _, processor := range s.serviceProcessors {
		log := processor(data, "create")
		if log != nil {
			s.collector.AddRawLog(log)
		}
	}
}

func (s *serviceCollector) HandleServiceUpdate(data *k8smeta.ObjectWrapper) {
	for _, processor := range s.serviceProcessors {
		log := processor(data, "update")
		if log != nil {
			s.collector.AddRawLog(log)
		}
	}
}

func (s *serviceCollector) HandleServiceDelete(data *k8smeta.ObjectWrapper) {
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

func (s *serviceCollector) processServiceEntity(data *k8smeta.ObjectWrapper, method string) *protocol.Log {
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

		return log
	}
	return nil
}
