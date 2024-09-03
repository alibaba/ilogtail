package k8smeta

import (
	"strings"

	app "k8s.io/api/apps/v1"
	batch "k8s.io/api/batch/v1"
	v1 "k8s.io/api/core/v1"
	"k8s.io/apimachinery/pkg/labels"
)

type K8sMetaLinkGenerator struct {
	metaCache map[string]MetaCache
}

func NewK8sMetaLinkGenerator(metaCache map[string]MetaCache) *K8sMetaLinkGenerator {
	return &K8sMetaLinkGenerator{
		metaCache: metaCache,
	}
}

func (p *K8sMetaLinkGenerator) GenerateLinks(events []*K8sMetaEvent, linkType string) []*K8sMetaEvent {
	if events == nil || len(events) == 0 {
		return nil
	}
	resourceType := events[0].Object.ResourceType
	// only generate link from the src entity
	if !strings.HasPrefix(linkType, resourceType) {
		return nil
	}
	switch linkType {
	case POD_NODE:
		return p.getPodNodeLink(events)
	case REPLICASET_DEPLOYMENT:
		return p.getReplicaSetDeploymentLink(events)
	case POD_REPLICASET, POD_STATEFULSET, POD_DAEMONSET, POD_JOB:
		return p.getParentPodLink(events)
	case JOB_CRONJOB:
		return p.getJobCronJobLink(events)
	case POD_PERSISENTVOLUMECLAIN:
		return p.getPodPVCLink(events)
	case POD_CONFIGMAP:
		return p.getPodConfigMapLink(events)
	case POD_SECRET:
		return p.getPodSecretLink(events)
	case POD_SERVICE:
		return p.getPodServiceLink(events)
	case POD_CONTAINER:
		return p.getPodContainerLink(events)
	default:
		return nil
	}
}

func (m *K8sMetaLinkGenerator) getPodNodeLink(events []*K8sMetaEvent) []*K8sMetaEvent {
	nodeCache := m.metaCache[NODE]
	result := make([]*K8sMetaEvent, 0)
	for _, event := range events {
		pod, ok := event.Object.Raw.(*v1.Pod)
		if !ok {
			continue
		}
		nodes := nodeCache.Get([]string{pod.Spec.NodeName})
		for _, node := range nodes {
			for _, n := range node {
				result = append(result, &K8sMetaEvent{
					EventType: event.EventType,
					Object: &ObjectWrapper{
						ResourceType: POD_NODE,
						Raw: &NodePod{
							Node: n.Raw.(*v1.Node),
							Pod:  pod,
						},
						FirstObservedTime: event.Object.FirstObservedTime,
						LastObservedTime:  event.Object.LastObservedTime,
					},
				})
			}
		}
	}
	return result
}

func (m *K8sMetaLinkGenerator) getReplicaSetDeploymentLink(events []*K8sMetaEvent) []*K8sMetaEvent {
	result := make([]*K8sMetaEvent, 0)
	for _, event := range events {
		replicaset, ok := event.Object.Raw.(*app.ReplicaSet)
		if !ok || len(replicaset.OwnerReferences) == 0 {
			continue
		}
		deploymentName := replicaset.OwnerReferences[0].Name
		deployments := m.metaCache[DEPLOYMENT].Get([]string{generateNameWithNamespaceKey(replicaset.Namespace, deploymentName)})
		for _, deployment := range deployments {
			for _, d := range deployment {
				result = append(result, &K8sMetaEvent{
					EventType: event.EventType,
					Object: &ObjectWrapper{
						ResourceType: REPLICASET_DEPLOYMENT,
						Raw: &ReplicaSetDeployment{
							Deployment: d.Raw.(*app.Deployment),
							ReplicaSet: replicaset,
						},
						FirstObservedTime: event.Object.FirstObservedTime,
						LastObservedTime:  event.Object.LastObservedTime,
					},
				})
			}
		}
	}
	return result
}

func (m *K8sMetaLinkGenerator) getParentPodLink(podList []*K8sMetaEvent) []*K8sMetaEvent {
	result := make([]*K8sMetaEvent, 0)
	for _, data := range podList {
		pod, ok := data.Object.Raw.(*v1.Pod)
		if !ok || len(pod.OwnerReferences) == 0 {
			continue
		}
		parentName := pod.OwnerReferences[0].Name
		switch pod.OwnerReferences[0].Kind {
		case "ReplicaSet":
			rsList := m.metaCache[REPLICASET].Get([]string{generateNameWithNamespaceKey(pod.Namespace, parentName)})
			for _, rs := range rsList {
				for _, r := range rs {
					result = append(result, &K8sMetaEvent{
						EventType: data.EventType,
						Object: &ObjectWrapper{
							ResourceType: POD_REPLICASET,
							Raw: &PodReplicaSet{
								ReplicaSet: r.Raw.(*app.ReplicaSet),
								Pod:        pod,
							},
							FirstObservedTime: data.Object.FirstObservedTime,
							LastObservedTime:  data.Object.LastObservedTime,
						},
					})
				}
			}
		case "StatefulSet":
			ssList := m.metaCache[STATEFULSET].Get([]string{generateNameWithNamespaceKey(pod.Namespace, parentName)})
			for _, ss := range ssList {
				for _, s := range ss {
					result = append(result, &K8sMetaEvent{
						EventType: data.EventType,
						Object: &ObjectWrapper{
							ResourceType: POD_STATEFULSET,
							Raw: &PodStatefulSet{
								StatefulSet: s.Raw.(*app.StatefulSet),
								Pod:         pod,
							},
							FirstObservedTime: data.Object.FirstObservedTime,
							LastObservedTime:  data.Object.LastObservedTime,
						},
					})
				}
			}
		case "DaemonSet":
			dsList := m.metaCache[DAEMONSET].Get([]string{generateNameWithNamespaceKey(pod.Namespace, parentName)})
			for _, ds := range dsList {
				for _, d := range ds {
					result = append(result, &K8sMetaEvent{
						EventType: data.EventType,
						Object: &ObjectWrapper{
							ResourceType: POD_DAEMONSET,
							Raw: &PodDaemonSet{
								DaemonSet: d.Raw.(*app.DaemonSet),
								Pod:       pod,
							},
							FirstObservedTime: data.Object.FirstObservedTime,
							LastObservedTime:  data.Object.LastObservedTime,
						},
					})
				}
			}
		case "Job":
			jobList := m.metaCache[JOB].Get([]string{generateNameWithNamespaceKey(pod.Namespace, parentName)})
			for _, job := range jobList {
				for _, j := range job {
					result = append(result, &K8sMetaEvent{
						EventType: data.EventType,
						Object: &ObjectWrapper{
							ResourceType: POD_JOB,
							Raw: &PodJob{
								Job: j.Raw.(*batch.Job),
								Pod: pod,
							},
							FirstObservedTime: data.Object.FirstObservedTime,
							LastObservedTime:  data.Object.LastObservedTime,
						},
					})
				}
			}
		}
	}
	return result
}

func (m *K8sMetaLinkGenerator) getJobCronJobLink(jobList []*K8sMetaEvent) []*K8sMetaEvent {
	result := make([]*K8sMetaEvent, 0)
	for _, data := range jobList {
		job, ok := data.Object.Raw.(*batch.Job)
		if !ok || len(job.OwnerReferences) == 0 {
			continue
		}
		cronJobName := job.OwnerReferences[0].Name
		cronJobList := m.metaCache[CRONJOB].Get([]string{generateNameWithNamespaceKey(job.Namespace, cronJobName)})
		for _, cj := range cronJobList {
			for _, c := range cj {
				result = append(result, &K8sMetaEvent{
					EventType: data.EventType,
					Object: &ObjectWrapper{
						ResourceType: JOB_CRONJOB,
						Raw: &JobCronJob{
							CronJob: c.Raw.(*batch.CronJob),
							Job:     job,
						},
						FirstObservedTime: data.Object.FirstObservedTime,
						LastObservedTime:  data.Object.LastObservedTime,
					},
				})
			}
		}
	}
	return result
}

func (m *K8sMetaLinkGenerator) getPodPVCLink(podList []*K8sMetaEvent) []*K8sMetaEvent {
	result := make([]*K8sMetaEvent, 0)
	for _, data := range podList {
		pod, ok := data.Object.Raw.(*v1.Pod)
		if !ok {
			continue
		}
		for _, volume := range pod.Spec.Volumes {
			if volume.PersistentVolumeClaim != nil {
				pvcName := volume.PersistentVolumeClaim.ClaimName
				pvcList := m.metaCache[PERSISTENTVOLUMECLAIM].Get([]string{generateNameWithNamespaceKey(pod.Namespace, pvcName)})
				for _, pvc := range pvcList {
					for _, p := range pvc {
						result = append(result, &K8sMetaEvent{
							EventType: data.EventType,
							Object: &ObjectWrapper{
								ResourceType: POD_PERSISENTVOLUMECLAIN,
								Raw: &PodPersistentVolumeClaim{
									Pod:                   pod,
									PersistentVolumeClaim: p.Raw.(*v1.PersistentVolumeClaim),
								},
								FirstObservedTime: data.Object.FirstObservedTime,
								LastObservedTime:  data.Object.LastObservedTime,
							},
						})
					}
				}
			}
		}
	}
	return result
}

func (m *K8sMetaLinkGenerator) getPodConfigMapLink(podList []*K8sMetaEvent) []*K8sMetaEvent {
	result := make([]*K8sMetaEvent, 0)
	for _, data := range podList {
		pod, ok := data.Object.Raw.(*v1.Pod)
		if !ok {
			continue
		}
		for _, volume := range pod.Spec.Volumes {
			if volume.ConfigMap != nil {
				cmName := volume.ConfigMap.Name
				cmList := m.metaCache[CONFIGMAP].Get([]string{generateNameWithNamespaceKey(pod.Namespace, cmName)})
				for _, cm := range cmList {
					for _, c := range cm {
						result = append(result, &K8sMetaEvent{
							EventType: data.EventType,
							Object: &ObjectWrapper{
								ResourceType: POD_CONFIGMAP,
								Raw: &PodConfigMap{
									Pod:       pod,
									ConfigMap: c.Raw.(*v1.ConfigMap),
								},
								FirstObservedTime: data.Object.FirstObservedTime,
								LastObservedTime:  data.Object.LastObservedTime,
							},
						})
					}
				}
			}
		}
	}
	return result
}

func (m *K8sMetaLinkGenerator) getPodSecretLink(podList []*K8sMetaEvent) []*K8sMetaEvent {
	result := make([]*K8sMetaEvent, 0)
	for _, data := range podList {
		pod, ok := data.Object.Raw.(*v1.Pod)
		if !ok {
			continue
		}
		for _, volume := range pod.Spec.Volumes {
			if volume.Secret != nil {
				secretName := volume.Secret.SecretName
				secretList := m.metaCache[SECRET].Get([]string{generateNameWithNamespaceKey(pod.Namespace, secretName)})
				for _, secret := range secretList {
					for _, s := range secret {
						result = append(result, &K8sMetaEvent{
							EventType: data.EventType,
							Object: &ObjectWrapper{
								ResourceType: POD_SECRET,
								Raw: &PodSecret{
									Pod:    pod,
									Secret: s.Raw.(*v1.Secret),
								},
								FirstObservedTime: data.Object.FirstObservedTime,
								LastObservedTime:  data.Object.LastObservedTime,
							},
						})
					}
				}
			}
		}
	}
	return result
}

func (m *K8sMetaLinkGenerator) getPodServiceLink(podList []*K8sMetaEvent) []*K8sMetaEvent {
	serviceList := m.metaCache[SERVICE].List()
	result := make([]*K8sMetaEvent, 0)
	matchers := make(map[string]labelMatchers)
	for _, data := range serviceList {
		service, ok := data.Raw.(*v1.Service)
		if !ok {
			continue
		}

		_, ok = matchers[service.Namespace]
		lm := newLabelMatcher(data.Raw, labels.SelectorFromSet(service.Spec.Selector))
		if !ok {
			matchers[service.Namespace] = []*labelMatcher{lm}
		} else {
			matchers[service.Namespace] = append(matchers[service.Namespace], lm)
		}
	}

	for _, data := range podList {
		pod, ok := data.Object.Raw.(*v1.Pod)
		if !ok {
			continue
		}
		nsSelectors, ok := matchers[pod.Namespace]
		if !ok {
			continue
		}
		set := labels.Set(pod.Labels)
		for _, s := range nsSelectors {
			if !s.selector.Empty() && s.selector.Matches(set) {
				result = append(result, &K8sMetaEvent{
					EventType: data.EventType,
					Object: &ObjectWrapper{
						ResourceType: POD_SERVICE,
						Raw: &PodService{
							Pod:     pod,
							Service: s.obj.(*v1.Service),
						},
						FirstObservedTime: data.Object.FirstObservedTime,
						LastObservedTime:  data.Object.LastObservedTime,
					},
				})
			}
		}
	}
	return result
}

func (m *K8sMetaLinkGenerator) getPodContainerLink(podList []*K8sMetaEvent) []*K8sMetaEvent {
	result := make([]*K8sMetaEvent, 0)
	for _, data := range podList {
		pod, ok := data.Object.Raw.(*v1.Pod)
		if !ok {
			continue
		}
		for _, container := range pod.Spec.Containers {
			result = append(result, &K8sMetaEvent{
				EventType: data.EventType,
				Object: &ObjectWrapper{
					ResourceType: POD_CONTAINER,
					Raw: &PodContainer{
						Pod:       pod,
						Container: &container,
					},
					FirstObservedTime: data.Object.FirstObservedTime,
					LastObservedTime:  data.Object.LastObservedTime,
				},
			})
		}
	}
	return result
}
