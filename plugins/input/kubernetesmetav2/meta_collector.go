package kubernetesmetav2

import (
	"context"

	// #nosec G501
	"crypto/md5"
	"fmt"
	"strconv"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

type metaCollector struct {
	serviceK8sMeta *ServiceK8sMeta
	collector      pipeline.Collector

	entityBuffer     chan models.PipelineEvent
	entityLinkBuffer chan models.PipelineEvent

	stopCh          chan struct{}
	entityProcessor map[string]ProcessFunc
}

func (m *metaCollector) Start() error {
	m.entityProcessor = map[string]ProcessFunc{
		k8smeta.POD:                      m.processPodEntity,
		k8smeta.NODE:                     m.processNodeEntity,
		k8smeta.SERVICE:                  m.processServiceEntity,
		k8smeta.DEPLOYMENT:               m.processDeploymentEntity,
		k8smeta.REPLICASET:               m.processReplicaSetEntity,
		k8smeta.DAEMONSET:                m.processDaemonSetEntity,
		k8smeta.STATEFULSET:              m.processStatefulSetEntity,
		k8smeta.CONFIGMAP:                m.processConfigMapEntity,
		k8smeta.SECRET:                   m.processSecretEntity,
		k8smeta.JOB:                      m.processJobEntity,
		k8smeta.CRONJOB:                  m.processCronJobEntity,
		k8smeta.NAMESPACE:                m.processNamespaceEntity,
		k8smeta.PERSISTENTVOLUME:         m.processPersistentVolumeEntity,
		k8smeta.PERSISTENTVOLUMECLAIM:    m.processPersistentVolumeClaimEntity,
		k8smeta.STORAGECLASS:             m.processStorageClassEntity,
		k8smeta.INGRESS:                  m.processIngressEntity,
		k8smeta.POD_NODE:                 m.processPodNodeLink,
		k8smeta.REPLICASET_DEPLOYMENT:    m.processReplicaSetDeploymentLink,
		k8smeta.POD_REPLICASET:           m.processPodReplicaSetLink,
		k8smeta.POD_STATEFULSET:          m.processPodStatefulSetLink,
		k8smeta.POD_DAEMONSET:            m.processPodDaemonSetLink,
		k8smeta.JOB_CRONJOB:              m.processJobCronJobLink,
		k8smeta.POD_JOB:                  m.processPodJobLink,
		k8smeta.POD_PERSISENTVOLUMECLAIN: m.processPodPVCLink,
		k8smeta.POD_CONFIGMAP:            m.processPodConfigMapLink,
		k8smeta.POD_SECRET:               m.processPodSecretLink,
		k8smeta.POD_SERVICE:              m.processPodServiceLink,
		k8smeta.POD_CONTAINER:            m.processPodContainerLink,
	}

	if m.serviceK8sMeta.Pod {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.POD, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.Node {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.NODE, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.Service {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.SERVICE, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.Deployment {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.DEPLOYMENT, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.ReplicaSet {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.REPLICASET, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.DaemonSet {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.DAEMONSET, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.StatefulSet {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.STATEFULSET, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.Configmap {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.CONFIGMAP, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.Secret {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.SECRET, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.Job {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.JOB, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.CronJob {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.CRONJOB, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.Namespace {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.NAMESPACE, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.PersistentVolume {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.PERSISTENTVOLUME, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.PersistentVolumeClaim {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.PERSISTENTVOLUMECLAIM, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.StorageClass {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.STORAGECLASS, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.Ingress {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.INGRESS, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.PodNodeLink && m.serviceK8sMeta.Pod && m.serviceK8sMeta.Node {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.POD_NODE, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.ReplicasetDeploymentLink && m.serviceK8sMeta.Deployment && m.serviceK8sMeta.ReplicaSet {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.REPLICASET_DEPLOYMENT, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.PodReplicaSetLink && m.serviceK8sMeta.ReplicaSet && m.serviceK8sMeta.Pod {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.POD_REPLICASET, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.PodStatefulSetLink && m.serviceK8sMeta.StatefulSet && m.serviceK8sMeta.Pod {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.POD_STATEFULSET, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.PodDaemonSetLink && m.serviceK8sMeta.DaemonSet && m.serviceK8sMeta.Pod {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.POD_DAEMONSET, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.JobCronJobLink && m.serviceK8sMeta.CronJob && m.serviceK8sMeta.Job {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.JOB_CRONJOB, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.PodJobLink && m.serviceK8sMeta.Job && m.serviceK8sMeta.Pod {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.POD_JOB, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.PodPvcLink && m.serviceK8sMeta.Pod && m.serviceK8sMeta.PersistentVolumeClaim {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.POD_PERSISENTVOLUMECLAIN, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.PodConfigMapLink && m.serviceK8sMeta.Pod && m.serviceK8sMeta.Configmap {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.POD_CONFIGMAP, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.PodSecretLink && m.serviceK8sMeta.Pod && m.serviceK8sMeta.Secret {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.POD_SECRET, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.PodServiceLink && m.serviceK8sMeta.Service && m.serviceK8sMeta.Pod {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.POD_SERVICE, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	if m.serviceK8sMeta.PodContainerLink && m.serviceK8sMeta.Pod {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.POD_CONTAINER, m.handleEvent, m.serviceK8sMeta.Interval)
	}
	go m.sendInBackground()
	return nil
}

func (m *metaCollector) Stop() error {
	if m.serviceK8sMeta.Pod {
		m.serviceK8sMeta.metaManager.UnRegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.POD)
	}
	if m.serviceK8sMeta.Service {
		m.serviceK8sMeta.metaManager.UnRegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.SERVICE)
	}
	if m.serviceK8sMeta.Deployment {
		m.serviceK8sMeta.metaManager.UnRegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.DEPLOYMENT)
	}
	if m.serviceK8sMeta.DaemonSet {
		m.serviceK8sMeta.metaManager.UnRegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.DAEMONSET)
	}
	if m.serviceK8sMeta.StatefulSet {
		m.serviceK8sMeta.metaManager.UnRegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.STATEFULSET)
	}
	if m.serviceK8sMeta.Configmap {
		m.serviceK8sMeta.metaManager.UnRegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.CONFIGMAP)
	}
	if m.serviceK8sMeta.Secret {
		m.serviceK8sMeta.metaManager.UnRegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.SECRET)
	}
	if m.serviceK8sMeta.Job {
		m.serviceK8sMeta.metaManager.UnRegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.JOB)
	}
	if m.serviceK8sMeta.CronJob {
		m.serviceK8sMeta.metaManager.UnRegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.CRONJOB)
	}
	if m.serviceK8sMeta.Namespace {
		m.serviceK8sMeta.metaManager.UnRegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.NAMESPACE)
	}
	if m.serviceK8sMeta.PersistentVolume {
		m.serviceK8sMeta.metaManager.UnRegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.PERSISTENTVOLUME)
	}
	if m.serviceK8sMeta.PersistentVolumeClaim {
		m.serviceK8sMeta.metaManager.UnRegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.PERSISTENTVOLUMECLAIM)
	}
	if m.serviceK8sMeta.StorageClass {
		m.serviceK8sMeta.metaManager.UnRegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.STORAGECLASS)
	}
	if m.serviceK8sMeta.Ingress {
		m.serviceK8sMeta.metaManager.UnRegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.INGRESS)
	}
	close(m.stopCh)
	return nil
}

func (m *metaCollector) handleEvent(event []*k8smeta.K8sMetaEvent) {
	if len(event) == 0 {
		return
	}
	switch event[0].EventType {
	case k8smeta.EventTypeAdd, k8smeta.EventTypeUpdate:
		for _, e := range event {
			m.handleAddOrUpdate(e)
		}
	case k8smeta.EventTypeDelete:
		for _, e := range event {
			m.handleDelete(e)
		}
	default:
		logger.Error(context.Background(), "UNKNOWN_EVENT_TYPE", "unknown event type", event[0].EventType)
	}
}

func (m *metaCollector) handleAddOrUpdate(event *k8smeta.K8sMetaEvent) {
	if processor, ok := m.entityProcessor[event.Object.ResourceType]; ok {
		logs := processor(event.Object, "Update")
		for _, log := range logs {
			m.send(log, isLink(event.Object.ResourceType))
			if !isLink(event.Object.ResourceType) {
				link := m.generateEntityClusterLink(log)
				m.send(link, true)
			}
		}
	}
}

func (m *metaCollector) handleDelete(event *k8smeta.K8sMetaEvent) {
	if processor, ok := m.entityProcessor[event.Object.ResourceType]; ok {
		logs := processor(event.Object, "Expire")
		for _, log := range logs {
			m.send(log, isLink(event.Object.ResourceType))
			if !isLink(event.Object.ResourceType) {
				link := m.generateEntityClusterLink(log)
				m.send(link, true)
			}
		}
	}
}

func (m *metaCollector) send(event models.PipelineEvent, entity bool) {
	var buffer chan models.PipelineEvent
	if entity {
		buffer = m.entityBuffer
	} else {
		buffer = m.entityLinkBuffer
	}
	buffer <- event
}

func (m *metaCollector) sendInBackground() {
	entityGroup := &models.PipelineGroupEvents{}
	entityLinkGroup := &models.PipelineGroupEvents{}
	sendFunc := func(group *models.PipelineGroupEvents) {
		for _, e := range group.Events {
			// TODO: temporary convert from event group back to log, will fix after pipeline support Go input to C++ processor
			log := convertPipelineEvent2Log(e)
			m.collector.AddRawLog(log)
		}
		group.Events = group.Events[:0]
	}
	for {
		select {
		case e := <-m.entityBuffer:
			entityGroup.Events = append(entityGroup.Events, e)
			if len(entityGroup.Events) >= 100 {
				sendFunc(entityGroup)
			}
		case e := <-m.entityLinkBuffer:
			entityLinkGroup.Events = append(entityLinkGroup.Events, e)
			if len(entityLinkGroup.Events) >= 100 {
				sendFunc(entityLinkGroup)
			}
		case <-time.After(3 * time.Second):
			if len(entityGroup.Events) > 0 {
				sendFunc(entityGroup)
			}
			if len(entityLinkGroup.Events) > 0 {
				sendFunc(entityLinkGroup)
			}
		case <-m.stopCh:
			return
		}
	}
}

func (m *metaCollector) genKey(namespace, name string) string {
	key := m.serviceK8sMeta.clusterID + namespace + name
	// #nosec G401
	return fmt.Sprintf("%x", md5.Sum([]byte(key)))
}

func (m *metaCollector) generateEntityClusterLink(entityEvent models.PipelineEvent) models.PipelineEvent {
	content := entityEvent.(*models.Log).Contents
	log := &models.Log{}
	log.Contents = models.NewLogContents()
	log.Contents.Add(entityLinkSrcDomainFieldName, m.serviceK8sMeta.Domain)
	log.Contents.Add(entityLinkSrcEntityTypeFieldName, content.Get(entityTypeFieldName))
	log.Contents.Add(entityLinkSrcEntityIDFieldName, content.Get(entityIDFieldName))

	log.Contents.Add(entityLinkDestDomainFieldName, m.serviceK8sMeta.Domain)
	log.Contents.Add(entityLinkDestEntityTypeFieldName, "ack.cluster")
	log.Contents.Add(entityLinkDestEntityIDFieldName, m.serviceK8sMeta.clusterID)

	log.Contents.Add(entityLinkRelationTypeFieldName, "runs")
	log.Contents.Add(entityMethodFieldName, content.Get(entityMethodFieldName))

	log.Contents.Add(entityFirstObservedTimeFieldName, content.Get(entityFirstObservedTimeFieldName))
	log.Contents.Add(entityLastObservedTimeFieldName, content.Get(entityLastObservedTimeFieldName))
	log.Contents.Add(entityKeepAliveSecondsFieldName, m.serviceK8sMeta.Interval*2)
	log.Contents.Add(entityCategoryFieldName, defaultEntityLinkCategory)
	log.Contents.Add(entityClusterIDFieldName, m.serviceK8sMeta.clusterID)
	log.Timestamp = uint64(time.Now().Unix())
	return log
}

func convertPipelineEvent2Log(event models.PipelineEvent) *protocol.Log {
	if modelLog, ok := event.(*models.Log); ok {
		log := &protocol.Log{}
		log.Contents = make([]*protocol.Log_Content, 0)
		for k, v := range modelLog.Contents.Iterator() {
			if _, ok := v.(string); !ok {
				if intValue, ok := v.(int); !ok {
					logger.Error(context.Background(), "COVERT_EVENT_TO_LOG_FAIL", "convert event to log fail, value is not string", v, "key", k)
					continue
				} else {
					v = strconv.Itoa(intValue)
				}
			}
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: k, Value: v.(string)})
		}
		protocol.SetLogTime(log, uint32(modelLog.GetTimestamp()))
		return log
	}
	return nil
}

func isLink(resourceType string) bool {
	return strings.Contains(resourceType, k8smeta.LINK_SPLIT_CHARACTER)
}
