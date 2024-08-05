package service

import (
	"context"
	"crypto/sha256"
	"fmt"
	"strconv"
	"strings"
	"time"

	v1 "k8s.io/api/core/v1"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

const defaultKeepAliveSeconds = "3600"

type metaCollector struct {
	serviceK8sMeta *ServiceK8sMeta
	processors     map[string][]ProcessFunc
	collector      pipeline.Collector

	eventCh          chan *k8smeta.K8sMetaEvent
	entityBuffer     *models.PipelineGroupEvents
	entityLinkBuffer *models.PipelineGroupEvents
}

func (m *metaCollector) Start() error {
	if m.serviceK8sMeta.Pod {
		m.serviceK8sMeta.metaManager.RegisterFlush(m.eventCh, m.serviceK8sMeta.configName, k8smeta.POD)
		m.processors[k8smeta.POD] = append(m.processors[k8smeta.POD], m.processPodEntity)
		m.processors[k8smeta.POD] = append(m.processors[k8smeta.POD], m.processPodReplicasetLink)
		if m.serviceK8sMeta.PodServiceLink {
			m.serviceK8sMeta.metaManager.RegisterFlush(m.eventCh, m.serviceK8sMeta.configName, k8smeta.POD_SERVICE)
			m.processors[k8smeta.POD_SERVICE] = append(m.processors[k8smeta.POD_SERVICE], m.processPodServiceLink)
		}
	}
	if m.serviceK8sMeta.Service {
		m.serviceK8sMeta.metaManager.RegisterFlush(m.eventCh, m.serviceK8sMeta.configName, k8smeta.SERVICE)
		m.processors[k8smeta.SERVICE] = append(m.processors[k8smeta.SERVICE], m.processServiceEntity)
	}
	m.handleEvent()
	return nil
}

func (m *metaCollector) Stop() error {
	if m.serviceK8sMeta.Pod {
		m.serviceK8sMeta.metaManager.UnRegisterFlush(m.serviceK8sMeta.configName, k8smeta.POD)
	}
	if m.serviceK8sMeta.Service {
		m.serviceK8sMeta.metaManager.UnRegisterFlush(m.serviceK8sMeta.configName, k8smeta.SERVICE)
	}
	return nil
}

func (m *metaCollector) handleEvent() {
	go func() {
		for {
			select {
			case data := <-m.eventCh:
				switch data.EventType {
				case k8smeta.EventTypeAdd:
					m.handleAdd(data)
				case k8smeta.EventTypeDelete:
					m.handleDelete(data)
				default:
					m.handleUpdate(data)
				}
			case <-time.After(time.Second):
				m.sendWithBuffer(nil, true)
				m.sendWithBuffer(nil, false)
			}
		}
	}()
}

func (m *metaCollector) handleAdd(data *k8smeta.K8sMetaEvent) {
	for _, processor := range m.processors[data.ResourceType] {
		log := processor(data, "create")
		if log != nil {
			m.sendWithBuffer(log, isLink(data.ResourceType))
		}
	}
}

func (m *metaCollector) handleUpdate(data *k8smeta.K8sMetaEvent) {
	for _, processor := range m.processors[data.ResourceType] {
		log := processor(data, "update")
		if log != nil {
			m.sendWithBuffer(log, isLink(data.ResourceType))
		}
	}
}

func (m *metaCollector) handleDelete(data *k8smeta.K8sMetaEvent) {
	for _, processor := range m.processors[data.ResourceType] {
		log := processor(data, "delete")
		if log != nil {
			m.sendWithBuffer(log, isLink(data.ResourceType))
		}
	}
}

func (m *metaCollector) sendWithBuffer(event models.PipelineEvent, entity bool) {
	var buffer *models.PipelineGroupEvents
	if entity {
		buffer = m.entityBuffer
	} else {
		buffer = m.entityLinkBuffer
	}
	if event != nil {
		buffer.Events = append(buffer.Events, event)
	}
	if event == nil || len(buffer.Events) >= 100 {
		// TODO: temporary convert from event group back to log, will fix after pipeline support Go input to C++ processor
		for _, e := range buffer.Events {
			log := convertPipelineEvent2Log(e)
			m.collector.AddRawLog(log)
		}
		buffer.Events = buffer.Events[:0]
	}
}

func convertPipelineEvent2Log(event models.PipelineEvent) *protocol.Log {
	if modelLog, ok := event.(*models.Log); ok {
		log := &protocol.Log{}
		log.Contents = make([]*protocol.Log_Content, 0)
		for k, v := range modelLog.Contents.Iterator() {
			if _, ok := v.(string); !ok {
				logger.Error(context.Background(), "COVERT_EVENT_TO_LOG_FAIL", "convert event to log fail, value is not string", v)
				continue
			}
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: k, Value: v.(string)})
		}
		protocol.SetLogTime(log, uint32(modelLog.GetTimestamp()))
		return log
	}
	return nil
}

func (m *metaCollector) processPodEntity(data *k8smeta.K8sMetaEvent, method string) models.PipelineEvent {
	if obj, ok := data.RawObject.(*v1.Pod); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add("__domain__", "k8s")
		log.Contents.Add("__entity_type__", "pod")
		log.Contents.Add("__entity_id__", genKeyByPod(obj))
		log.Contents.Add("__method__", method)

		log.Contents.Add("__first_observed_time__", strconv.FormatInt(data.CreateTime, 10))
		log.Contents.Add("__last_observed_time__", strconv.FormatInt(data.UpdateTime, 10))
		log.Contents.Add("__keep_alive_seconds__", defaultKeepAliveSeconds)

		log.Contents.Add("namespace", obj.Namespace)
		log.Contents.Add("name", obj.Name)
		log.Contents.Add("__category__", "entity")
		log.Timestamp = uint64(time.Now().Unix())
		return log
	}
	return nil
}

func (m *metaCollector) processPodReplicasetLink(data *k8smeta.K8sMetaEvent, method string) models.PipelineEvent {
	if obj, ok := data.RawObject.(*v1.Pod); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add("__src_domain__", "k8s")
		log.Contents.Add("__src_entity_type__", "pod")
		log.Contents.Add("__src_entity_id__", genKeyByPod(obj))

		if len(obj.OwnerReferences) > 0 {
			ownerReferences := obj.OwnerReferences[0]
			log.Contents.Add("__dest_domain__", "k8s")
			log.Contents.Add("__dest_entity_type__", "replicaset")
			log.Contents.Add("__dest_entity_id__", genKey(obj.Namespace, ownerReferences.Kind, ownerReferences.Name))
		} else {
			// 数据不完整就没有意义
			return nil
		}

		log.Contents.Add("__relation_type__", "contain")

		switch method {
		case "create", "update":
			log.Contents.Add("__method__", "update")
		case "delete":
			log.Contents.Add("__method__", method)
		default:
			// 数据不完整就没有意义
			return nil
		}

		log.Contents.Add("__first_observed_time__", strconv.FormatInt(data.CreateTime, 10))
		log.Contents.Add("__last_observed_time__", strconv.FormatInt(data.UpdateTime, 10))
		log.Contents.Add("__keep_alive_seconds__", defaultKeepAliveSeconds)
		log.Contents.Add("__category__", "entity_link")
		log.Timestamp = uint64(time.Now().Unix())
		return log
	}
	return nil
}

func (m *metaCollector) processPodServiceLink(data *k8smeta.K8sMetaEvent, method string) models.PipelineEvent {
	if obj, ok := data.RawObject.(*k8smeta.PodService); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add("__src_domain__", "k8s")
		log.Contents.Add("__src_entity_type__", "pod")
		log.Contents.Add("__src_entity_id__", genKeyByPod(obj.Pod))

		log.Contents.Add("__dest_domain__", "k8s")
		log.Contents.Add("__dest_entity_type__", "service")
		log.Contents.Add("__dest_entity_id__", genKeyByService(obj.Service))

		log.Contents.Add("__relation_type__", "link")

		switch method {
		case "create", "update":
			log.Contents.Add("__method__", "update")
		case "delete":
			log.Contents.Add("__method__", method)
		default:
			// 数据不完整就没有意义
			return nil
		}

		log.Contents.Add("__first_observed_time__", strconv.FormatInt(data.CreateTime, 10))
		log.Contents.Add("__last_observed_time__", strconv.FormatInt(data.UpdateTime, 10))
		log.Contents.Add("__keep_alive_seconds__", defaultKeepAliveSeconds)
		log.Contents.Add("__category__", "entity_link")
		log.Timestamp = uint64(time.Now().Unix())
		return log
	}
	return nil
}

func (m *metaCollector) processServiceEntity(data *k8smeta.K8sMetaEvent, method string) models.PipelineEvent {
	if obj, ok := data.RawObject.(*v1.Service); ok {
		log := &models.Log{}
		log.Contents = models.NewLogContents()
		log.Contents.Add("__domain__", "k8s")
		log.Contents.Add("__entity_type__", "service")
		log.Contents.Add("__entity_id__", genKeyByService(obj))
		log.Contents.Add("__method__", method)

		log.Contents.Add("__first_observed_time__", strconv.FormatInt(data.CreateTime, 10))
		log.Contents.Add("__last_observed_time__", strconv.FormatInt(data.UpdateTime, 10))
		log.Contents.Add("__keep_alive_seconds__", defaultKeepAliveSeconds)

		log.Contents.Add("namespace", obj.Namespace)
		log.Contents.Add("name", obj.Name)
		log.Contents.Add("__category__", "entity")
		log.Timestamp = uint64(time.Now().Unix())
		return log
	}
	return nil
}

func genKeyByPod(pod *v1.Pod) string {
	return genKey(pod.Namespace, pod.Kind, pod.Name)
}

func genKey(namespace, kind, name string) string {
	key := namespace + kind + name
	return fmt.Sprintf("%x", sha256.Sum256([]byte(key)))
}

func genKeyByService(service *v1.Service) string {
	return genKey(service.Namespace, service.Kind, service.Name)
}

func isLink(resourceType string) bool {
	return strings.Contains(resourceType, k8smeta.LINK_SPLIT_CHARACTER)
}
