package service

import (
	"crypto/md5"
	"fmt"
	"strconv"
	"time"

	v1 "k8s.io/api/core/v1"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

type podCollector struct {
	serviceK8sMeta *ServiceK8sMeta
	podProcessors  []ProcessFunc
	metaManager    *k8smeta.MetaManager
	collector      pipeline.Collector

	podCh        chan *k8smeta.K8sMetaEvent
	podServiceCh chan *k8smeta.K8sMetaEvent
}

func (p *podCollector) init() {
	p.registerInnerProcessor()
	if p.serviceK8sMeta.Pod {
		p.podCh = make(chan *k8smeta.K8sMetaEvent, 100)
		p.metaManager.RegisterFlush(p.podCh, p.serviceK8sMeta.configName, k8smeta.POD)
		if p.serviceK8sMeta.PodServiceLink {
			p.podServiceCh = make(chan *k8smeta.K8sMetaEvent, 100)
			p.metaManager.RegisterFlush(p.podServiceCh, p.serviceK8sMeta.configName, k8smeta.POD_SERVICE)
		}
		p.handlePodEvent()
	}
}

func (s *podCollector) handlePodEvent() {
	go func() {
		for {
			select {
			case data := <-s.podCh:
				switch data.EventType {
				case k8smeta.EventTypeAdd:
					s.HandlePodAdd(data)
				case k8smeta.EventTypeDelete:
					s.HandlePodDelete(data)
				default:
					s.HandlePodUpdate(data)
				}
			case data := <-s.podServiceCh:
				s.HandlePodService(data)
			}
		}
	}()
}

func (s *podCollector) HandlePodAdd(data *k8smeta.K8sMetaEvent) {
	for _, processor := range s.podProcessors {
		log := processor(data, "create")
		if log != nil {
			s.collector.AddRawLog(log)
		}
	}
}

func (s *podCollector) HandlePodUpdate(data *k8smeta.K8sMetaEvent) {
	for _, processor := range s.podProcessors {
		log := processor(data, "update")
		if log != nil {
			s.collector.AddRawLog(log)
		}
	}
}

func (s *podCollector) HandlePodDelete(data *k8smeta.K8sMetaEvent) {
	for _, processor := range s.podProcessors {
		log := processor(data, "delete")
		if log != nil {
			s.collector.AddRawLog(log)
		}
	}
}

func (s *podCollector) HandlePodService(data *k8smeta.K8sMetaEvent) {
	log := s.processPodServiceLink(data, "update")
	if log != nil {
		s.collector.AddRawLog(log)
	}
}

func genKeyByPod(pod *v1.Pod) string {
	return genKey(pod.Namespace, pod.Kind, pod.Name)
}

func genKey(namespace, kind, name string) string {
	key := namespace + kind + name
	return fmt.Sprintf("%x", md5.Sum([]byte(key)))
}

func (s *podCollector) registerInnerProcessor() {
	if s.serviceK8sMeta.Pod {
		s.podProcessors = append(s.podProcessors, s.processPodEntity)
	}
	if s.serviceK8sMeta.PodReplicasetLink {
		s.podProcessors = append(s.podProcessors, s.processPodReplicasetLink)
	}
}

func (s *podCollector) processPodEntity(data *k8smeta.K8sMetaEvent, method string) *protocol.Log {
	log := &protocol.Log{}
	if obj, ok := data.RawObject.(*v1.Pod); ok {
		log.Contents = make([]*protocol.Log_Content, 0)
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__domain__", Value: "k8s"})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__entity_type__", Value: "pod"})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__entity_id__", Value: genKeyByPod(obj)})
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

func (s *podCollector) processPodReplicasetLink(data *k8smeta.K8sMetaEvent, method string) *protocol.Log {
	log := &protocol.Log{}
	if obj, ok := data.RawObject.(*v1.Pod); ok {
		log.Contents = make([]*protocol.Log_Content, 0)
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__src_domain__", Value: "k8s"})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__src_entity_type__", Value: "pod"})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__src_entity_id__", Value: genKeyByPod(obj)})

		if len(obj.OwnerReferences) > 0 {
			ownerReferences := obj.OwnerReferences[0]
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__dest_domain__", Value: "k8s"})
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__dest_entity_type__", Value: "replicaset"})
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__dest_entity_id__", Value: genKey(obj.Namespace, ownerReferences.Kind, ownerReferences.Name)})
		} else {
			// 数据不完整就没有意义
			return nil
		}

		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__relation_type__", Value: "contain"})

		if method == "create" || method == "update" {
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__method__", Value: "update"})
		} else if method == "delete" {
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__method__", Value: method})
		} else {
			// 数据不完整就没有意义
			return nil
		}

		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__first_observed_time__", Value: strconv.FormatInt(data.CreateTime, 10)})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__last_observed_time__", Value: strconv.FormatInt(data.UpdateTime, 10)})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__keep_alive_seconds__", Value: "3600"})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__category__", Value: "entity_link"})
		protocol.SetLogTime(log, uint32(time.Now().Unix()))
		return log
	}
	return nil
}

func (s *podCollector) processPodServiceLink(data *k8smeta.K8sMetaEvent, method string) *protocol.Log {
	log := &protocol.Log{}
	if obj, ok := data.RawObject.(*k8smeta.PodService); ok {
		log.Contents = make([]*protocol.Log_Content, 0)
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__src_domain__", Value: "k8s"})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__src_entity_type__", Value: "pod"})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__src_entity_id__", Value: genKeyByPod(obj.Pod)})

		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__dest_domain__", Value: "k8s"})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__dest_entity_type__", Value: "service"})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__dest_entity_id__", Value: genKeyByService(obj.Service)})

		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__relation_type__", Value: "contain"})

		if method == "create" || method == "update" {
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__method__", Value: "update"})
		} else if method == "delete" {
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__method__", Value: method})
		} else {
			// 数据不完整就没有意义
			return nil
		}
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__first_observed_time__", Value: strconv.FormatInt(data.CreateTime, 10)})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__last_observed_time__", Value: strconv.FormatInt(data.UpdateTime, 10)})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__keep_alive_seconds__", Value: "3600"})
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__category__", Value: "entity_link"})
		protocol.SetLogTime(log, uint32(time.Now().Unix()))
		return log
	}
	return nil
}
