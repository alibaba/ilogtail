package k8smeta

import v1 "k8s.io/api/core/v1"

const POD = "pod"

const SERVICE = "service"

type PodService struct {
	Pod     *v1.Pod
	Service *v1.Service
}
