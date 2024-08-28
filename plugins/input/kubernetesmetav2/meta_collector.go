package kubernetesmetav2

import (
	"context"

	// #nosec G401
	"crypto/md5"
	"fmt"
	"os"
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
	processors     map[string][]ProcessFunc
	collector      pipeline.Collector

	entityBuffer     chan models.PipelineEvent
	entityLinkBuffer chan models.PipelineEvent

	stopCh chan struct{}
}

func (m *metaCollector) Start() error {
	if m.serviceK8sMeta.Pod {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.POD, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.POD] = append(m.processors[k8smeta.POD], m.processPodEntity)
	}
	if m.serviceK8sMeta.Node {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.NODE, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.NODE] = append(m.processors[k8smeta.NODE], m.processNodeEntity)
	}
	if m.serviceK8sMeta.Service {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.SERVICE, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.SERVICE] = append(m.processors[k8smeta.SERVICE], m.processServiceEntity)
	}
	if m.serviceK8sMeta.Deployment {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.DEPLOYMENT, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.DEPLOYMENT] = append(m.processors[k8smeta.DEPLOYMENT], m.processDeploymentEntity)
	}
	if m.serviceK8sMeta.ReplicaSet {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.REPLICASET, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.REPLICASET] = append(m.processors[k8smeta.REPLICASET], m.processReplicaSetEntity)
	}
	if m.serviceK8sMeta.DaemonSet {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.DAEMONSET, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.DAEMONSET] = append(m.processors[k8smeta.DAEMONSET], m.processDaemonSetEntity)
	}
	if m.serviceK8sMeta.StatefulSet {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.STATEFULSET, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.STATEFULSET] = append(m.processors[k8smeta.STATEFULSET], m.processStatefulSetEntity)
	}
	if m.serviceK8sMeta.Configmap {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.CONFIGMAP, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.CONFIGMAP] = append(m.processors[k8smeta.CONFIGMAP], m.processConfigMapEntity)
	}
	if m.serviceK8sMeta.Secret {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.SECRET, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.SECRET] = append(m.processors[k8smeta.SECRET], m.processSecretEntity)
	}
	if m.serviceK8sMeta.Job {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.JOB, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.JOB] = append(m.processors[k8smeta.JOB], m.processJobEntity)
	}
	if m.serviceK8sMeta.CronJob {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.CRONJOB, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.CRONJOB] = append(m.processors[k8smeta.CRONJOB], m.processCronJobEntity)
	}
	if m.serviceK8sMeta.Namespace {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.NAMESPACE, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.NAMESPACE] = append(m.processors[k8smeta.NAMESPACE], m.processNamespaceEntity)
	}
	if m.serviceK8sMeta.PersistentVolume {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.PERSISTENTVOLUME, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.PERSISTENTVOLUME] = append(m.processors[k8smeta.PERSISTENTVOLUME], m.processPersistentVolumeEntity)
	}
	if m.serviceK8sMeta.PersistentVolumeClaim {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.PERSISTENTVOLUMECLAIM, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.PERSISTENTVOLUMECLAIM] = append(m.processors[k8smeta.PERSISTENTVOLUMECLAIM], m.processPersistentVolumeClaimEntity)
	}
	if m.serviceK8sMeta.StorageClass {
		m.serviceK8sMeta.metaManager.RegisterSendFunc(m.serviceK8sMeta.configName, k8smeta.STORAGECLASS, m.handleEvent, m.serviceK8sMeta.Interval)
		m.processors[k8smeta.STORAGECLASS] = append(m.processors[k8smeta.STORAGECLASS], m.processStorageClassEntity)
	}
	if m.serviceK8sMeta.Ingress {
		m.processors[k8smeta.INGRESS] = append(m.processors[k8smeta.INGRESS], m.processIngressEntity)
	}
	if m.serviceK8sMeta.NodePodLink && m.serviceK8sMeta.Pod && m.serviceK8sMeta.Node {
		m.processors[k8smeta.NODE_POD] = append(m.processors[k8smeta.NODE_POD], m.processNodePodLink)
	}
	if m.serviceK8sMeta.DeploymentReplicasetLink && m.serviceK8sMeta.Deployment && m.serviceK8sMeta.ReplicaSet {
		m.processors[k8smeta.DEPLOYMENT_REPLICASET] = append(m.processors[k8smeta.DEPLOYMENT_REPLICASET], m.processDeploymentReplicaSetLink)
	}
	if m.serviceK8sMeta.ReplicaSetPodLink && m.serviceK8sMeta.ReplicaSet && m.serviceK8sMeta.Pod {
		m.processors[k8smeta.REPLICASET_POD] = append(m.processors[k8smeta.REPLICASET_POD], m.processReplicaSetPodLink)
	}
	if m.serviceK8sMeta.StatefulSetPodLink && m.serviceK8sMeta.StatefulSet && m.serviceK8sMeta.Pod {
		m.processors[k8smeta.STATEFULSET_POD] = append(m.processors[k8smeta.STATEFULSET_POD], m.processStatefulSetPodLink)
	}
	if m.serviceK8sMeta.DaemonSetPodLink && m.serviceK8sMeta.DaemonSet && m.serviceK8sMeta.Pod {
		m.processors[k8smeta.DAEMONSET_POD] = append(m.processors[k8smeta.DAEMONSET_POD], m.processDaemonSetPodLink)
	}
	if m.serviceK8sMeta.CronjobJobLink && m.serviceK8sMeta.CronJob && m.serviceK8sMeta.Job {
		m.processors[k8smeta.CRONJOB_JOB] = append(m.processors[k8smeta.CRONJOB_JOB], m.processCronJobJobLink)
	}
	if m.serviceK8sMeta.JobPodLink && m.serviceK8sMeta.Job && m.serviceK8sMeta.Pod {
		m.processors[k8smeta.JOB_POD] = append(m.processors[k8smeta.JOB_POD], m.processJobPodLink)
	}
	if m.serviceK8sMeta.PodPvcLink && m.serviceK8sMeta.Pod && m.serviceK8sMeta.PersistentVolumeClaim {
		m.processors[k8smeta.POD_PERSISENTVOLUMECLAIN] = append(m.processors[k8smeta.POD_PERSISENTVOLUMECLAIN], m.processPodPVCLink)
	}
	if m.serviceK8sMeta.PodConfigMapLink && m.serviceK8sMeta.Pod && m.serviceK8sMeta.Configmap {
		m.processors[k8smeta.POD_CONFIGMAP] = append(m.processors[k8smeta.POD_CONFIGMAP], m.processPodConfigMapLink)
	}
	if m.serviceK8sMeta.PodSecretLink && m.serviceK8sMeta.Pod && m.serviceK8sMeta.Secret {
		m.processors[k8smeta.POD_SECRET] = append(m.processors[k8smeta.POD_SECRET], m.processPodSecretLink)
	}
	if m.serviceK8sMeta.ServicePodLink && m.serviceK8sMeta.Service && m.serviceK8sMeta.Pod {
		m.processors[k8smeta.SERVICE_POD] = append(m.processors[k8smeta.SERVICE_POD], m.processServicePodLink)
	}
	if m.serviceK8sMeta.PodContainerLink && m.serviceK8sMeta.Pod {
		m.processors[k8smeta.POD_CONTAINER] = append(m.processors[k8smeta.POD_CONTAINER], m.processPodContainerLink)
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
	case k8smeta.EventTypeAdd:
		for _, e := range event {
			m.handleAdd(e)
		}
	case k8smeta.EventTypeUpdate:
		for _, e := range event {
			m.handleUpdate(e)
		}
	case k8smeta.EventTypeDelete:
		for _, e := range event {
			m.handleDelete(e)
		}
	default:
		logger.Error(context.Background(), "UNKNOWN_EVENT_TYPE", "unknown event type", event[0].EventType)
	}
}

func (m *metaCollector) handleAdd(event *k8smeta.K8sMetaEvent) {
	if processors, ok := m.processors[event.Object.ResourceType]; ok {
		for _, processor := range processors {
			log := processor(event.Object, "create")
			if log != nil {
				m.send(log, isLink(event.Object.ResourceType))
			}
		}
	}
}

func (m *metaCollector) handleUpdate(event *k8smeta.K8sMetaEvent) {
	if processors, ok := m.processors[event.Object.ResourceType]; ok {
		for _, processor := range processors {
			log := processor(event.Object, "update")
			if log != nil {
				m.send(log, isLink(event.Object.ResourceType))
			}
		}
	}
}

func (m *metaCollector) handleDelete(event *k8smeta.K8sMetaEvent) {
	if processors, ok := m.processors[event.Object.ResourceType]; ok {
		for _, processor := range processors {
			log := processor(event.Object, "delete")
			if log != nil {
				m.send(log, isLink(event.Object.ResourceType))
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

func convertPipelineEvent2Log(event models.PipelineEvent) *protocol.Log {
	if modelLog, ok := event.(*models.Log); ok {
		log := &protocol.Log{}
		log.Contents = make([]*protocol.Log_Content, 0)
		for k, v := range modelLog.Contents.Iterator() {
			if _, ok := v.(string); !ok {
				if intValue, ok := v.(int); !ok {
					logger.Error(context.Background(), "COVERT_EVENT_TO_LOG_FAIL", "convert event to log fail, value is not string", v)
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

func genKey(namespace, name string) string {
	clusterId := os.Getenv("_cluster_id_")
	key := clusterId + namespace + name
	// #nosec G401
	return fmt.Sprintf("%x", md5.Sum([]byte(key)))
}

func isLink(resourceType string) bool {
	return strings.Contains(resourceType, k8smeta.LINK_SPLIT_CHARACTER)
}
