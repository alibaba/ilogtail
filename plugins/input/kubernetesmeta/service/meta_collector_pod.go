package service

import (
	"crypto/sha256"
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

func (p *podCollector) handlePodEvent() {
	go func() {
		for {
			select {
			case data := <-p.podCh:
				switch data.EventType {
				case k8smeta.EventTypeAdd:
					p.HandlePodAdd(data)
				case k8smeta.EventTypeDelete:
					p.HandlePodDelete(data)
				default:
					p.HandlePodUpdate(data)
				}
			case data := <-p.podServiceCh:
				p.HandlePodService(data)
			}
		}
	}()
}

func (p *podCollector) HandlePodAdd(data *k8smeta.K8sMetaEvent) {
	for _, processor := range p.podProcessors {
		log := processor(data, "create")
		if log != nil {
			p.collector.AddRawLog(log)
		}
	}
}

func (p *podCollector) HandlePodUpdate(data *k8smeta.K8sMetaEvent) {
	for _, processor := range p.podProcessors {
		log := processor(data, "update")
		if log != nil {
			p.collector.AddRawLog(log)
		}
	}
}

func (p *podCollector) HandlePodDelete(data *k8smeta.K8sMetaEvent) {
	for _, processor := range p.podProcessors {
		log := processor(data, "delete")
		if log != nil {
			p.collector.AddRawLog(log)
		}
	}
}

func (p *podCollector) HandlePodService(data *k8smeta.K8sMetaEvent) {
	log := p.processPodServiceLink(data, "update")
	if log != nil {
		p.collector.AddRawLog(log)
	}
}

func genKeyByPod(pod *v1.Pod) string {
	return genKey(pod.Namespace, pod.Kind, pod.Name)
}

func genKey(namespace, kind, name string) string {
	key := namespace + kind + name
	return fmt.Sprintf("%x", sha256.Sum256([]byte(key)))
}

func (p *podCollector) registerInnerProcessor() {
	if p.serviceK8sMeta.Pod {
		p.podProcessors = append(p.podProcessors, p.processPodEntity)
	}
	if p.serviceK8sMeta.PodReplicasetLink {
		p.podProcessors = append(p.podProcessors, p.processPodReplicasetLink)
	}
}

func (p *podCollector) processPodEntity(data *k8smeta.K8sMetaEvent, method string) *protocol.Log {
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

func (p *podCollector) processPodReplicasetLink(data *k8smeta.K8sMetaEvent, method string) *protocol.Log {
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

		switch method {
		case "create", "update":
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__method__", Value: "update"})
		case "delete":
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__method__", Value: method})
		default:
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

func (p *podCollector) processPodServiceLink(data *k8smeta.K8sMetaEvent, method string) *protocol.Log {
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

		switch method {
		case "create", "update":
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__method__", Value: "update"})
		case "delete":
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "__method__", Value: method})
		default:
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
