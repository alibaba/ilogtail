package kubernetesmetav2

import (
	"context"
	"crypto/sha256"
	"fmt"
	"strings"
	"time"

	v1 "k8s.io/api/core/v1"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

type metaCollector struct {
	serviceK8sMeta *ServiceK8sMeta
	processors     map[string][]ProcessFunc
	collector      pipeline.Collector

	eventCh          chan *k8smeta.K8sMetaEvent
	entityTypes      []string
	entityBuffer     *models.PipelineGroupEvents
	entityLinkBuffer *models.PipelineGroupEvents

	stopCh chan struct{}
}

func (m *metaCollector) Start() error {
	if m.serviceK8sMeta.Pod {
		m.serviceK8sMeta.metaManager.RegisterFlush(m.eventCh, m.serviceK8sMeta.configName, k8smeta.POD)
		m.processors[k8smeta.POD] = append(m.processors[k8smeta.POD], m.processPodEntity)
		m.processors[k8smeta.POD] = append(m.processors[k8smeta.POD], m.processPodReplicasetLink)
		m.entityTypes = append(m.entityTypes, k8smeta.POD)
	}
	if m.serviceK8sMeta.Service {
		m.serviceK8sMeta.metaManager.RegisterFlush(m.eventCh, m.serviceK8sMeta.configName, k8smeta.SERVICE)
		m.processors[k8smeta.SERVICE] = append(m.processors[k8smeta.SERVICE], m.processServiceEntity)
		m.entityTypes = append(m.entityTypes, k8smeta.SERVICE)
	}
	if m.serviceK8sMeta.Pod && m.serviceK8sMeta.Service && m.serviceK8sMeta.PodServiceLink {
		m.serviceK8sMeta.metaManager.RegisterFlush(m.eventCh, m.serviceK8sMeta.configName, k8smeta.POD_SERVICE)
		m.processors[k8smeta.POD_SERVICE] = append(m.processors[k8smeta.POD_SERVICE], m.processPodServiceLink)
		m.entityTypes = append(m.entityTypes, k8smeta.POD_SERVICE)
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
		defer panicRecover()
		for {
			if !m.serviceK8sMeta.metaManager.IsReady() {
				time.Sleep(time.Second)
				continue
			}
			break
		}
		// handle timer once at first
		m.handleTimer()
		ticker := time.NewTicker(time.Second * time.Duration(m.serviceK8sMeta.Interval))
		for {
			select {
			case <-m.stopCh:
				return
			case <-ticker.C:
				m.handleTimer()
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

func (m *metaCollector) handleAdd(event *k8smeta.K8sMetaEvent) {
	for _, processor := range m.processors[event.Object.ResourceType] {
		log := processor(event.Object, "create")
		if log != nil {
			m.sendWithBuffer(log, isLink(event.Object.ResourceType))
		}
	}
}

func (m *metaCollector) handleUpdate(event *k8smeta.K8sMetaEvent) {
	for _, processor := range m.processors[event.Object.ResourceType] {
		log := processor(event.Object, "update")
		if log != nil {
			m.sendWithBuffer(log, isLink(event.Object.ResourceType))
		}
	}
}

func (m *metaCollector) handleDelete(event *k8smeta.K8sMetaEvent) {
	for _, processor := range m.processors[event.Object.ResourceType] {
		log := processor(event.Object, "delete")
		if log != nil {
			m.sendWithBuffer(log, isLink(event.Object.ResourceType))
		}
	}
}

func (m *metaCollector) handleTimer() {
	metas := m.serviceK8sMeta.metaManager.List(m.entityTypes)
	for _, meta := range metas {
		m.handleUpdate(&k8smeta.K8sMetaEvent{
			EventType: k8smeta.EventTypeUpdate,
			Object:    meta,
		})
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
