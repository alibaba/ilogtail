package k8smeta

import (
	app "k8s.io/api/apps/v1"
	batch "k8s.io/api/batch/v1"
	v1 "k8s.io/api/core/v1"
)

const (
	// entity type
	POD                   = "pod"
	SERVICE               = "service"
	DEPLOYMENT            = "deployment"
	REPLICASET            = "replicaset"
	STATEFULSET           = "statefulset"
	DAEMONSET             = "daemonset"
	CRONJOB               = "cronjob"
	JOB                   = "job"
	NODE                  = "node"
	NAMESPACE             = "namespace"
	CONFIGMAP             = "configmap"
	SECRET                = "secret"
	PERSISTENTVOLUME      = "persistentvolume"
	PERSISTENTVOLUMECLAIM = "persistentvolumeclaim"
	STORAGECLASS          = "storageclass"
	INGRESS               = "ingress"
	CONTAINER             = "container"
	// entity link type
	//revive:disable:var-naming
	LINK_SPLIT_CHARACTER     = "->"
	NODE_POD                 = "node->pod"
	DEPLOYMENT_REPLICASET    = "deployment->replicaset"
	REPLICASET_POD           = "replicaset->pod"
	STATEFULSET_POD          = "statefulset->pod"
	DAEMONSET_POD            = "daemonset->pod"
	CRONJOB_JOB              = "cronjob->job"
	JOB_POD                  = "job->pod"
	POD_PERSISENTVOLUMECLAIN = "pod->persistentvolumeclaim"
	POD_CONFIGMAP            = "pod->configmap"
	POD_SECRET               = "pod->secret"
	SERVICE_POD              = "service->pod"
	POD_CONTAINER            = "pod->container"
	POD_PROCESS              = "pod->process"
	//revive:enable:var-naming
)

var ALL_LINK_MAP = map[string]bool{
	NODE_POD:                 true,
	DEPLOYMENT_REPLICASET:    true,
	REPLICASET_POD:           true,
	STATEFULSET_POD:          true,
	DAEMONSET_POD:            true,
	CRONJOB_JOB:              true,
	JOB_POD:                  true,
	POD_PERSISENTVOLUMECLAIN: true,
	POD_CONFIGMAP:            true,
	POD_SECRET:               true,
	SERVICE_POD:              true,
	POD_CONTAINER:            true,
	POD_PROCESS:              true,
}

type NodePod struct {
	Node *v1.Node
	Pod  *v1.Pod
}

type DeploymentReplicaSet struct {
	Deployment *app.Deployment
	ReplicaSet *app.ReplicaSet
}

type ReplicaSetPod struct {
	ReplicaSet *app.ReplicaSet
	Pod        *v1.Pod
}

type StatefulSetPod struct {
	StatefulSet *app.StatefulSet
	Pod         *v1.Pod
}

type DaemonSetPod struct {
	DaemonSet *app.DaemonSet
	Pod       *v1.Pod
}

type CronJobJob struct {
	CronJob *batch.CronJob
	Job     *batch.Job
}

type JobPod struct {
	Job *batch.Job
	Pod *v1.Pod
}

type PodPersistentVolumeClaim struct {
	Pod                   *v1.Pod
	PersistentVolumeClaim *v1.PersistentVolumeClaim
}

type PodConfigMap struct {
	Pod       *v1.Pod
	ConfigMap *v1.ConfigMap
}

type PodSecret struct {
	Pod    *v1.Pod
	Secret *v1.Secret
}

type ServicePod struct {
	Service *v1.Service
	Pod     *v1.Pod
}

type PodContainer struct {
	Pod       *v1.Pod
	Container *v1.Container
}

const (
	EventTypeAdd            = "add"
	EventTypeUpdate         = "update"
	EventTypeDelete         = "delete"
	EventTypeDeferredDelete = "deferredDelete"
	EventTypeTimer          = "timer"
)

type PodMetadata struct {
	Namespace    string            `json:"namespace"`
	WorkloadName string            `json:"workloadName"`
	WorkloadKind string            `json:"workloadKind"`
	ServiceName  string            `json:"serviceName"`
	Labels       map[string]string `json:"labels"`
	Envs         map[string]string `json:"envs"`
	Images       map[string]string `json:"images"`
	IsDeleted    bool              `json:"-"`
}
