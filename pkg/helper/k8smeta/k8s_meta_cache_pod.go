package k8smeta

import (
	"context"
	"fmt"
	"time"

	v1 "k8s.io/api/core/v1"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/client-go/informers"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/tools/cache"
	"sigs.k8s.io/controller-runtime/pkg/client/apiutil"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type podCache struct {
	metaStore        *DeferredDeletionMetaStore
	serviceMetaStore *DeferredDeletionMetaStore
	clientset        *kubernetes.Clientset

	eventCh chan *K8sMetaEvent
	stopCh  chan struct{}
	schema  *runtime.Scheme
}

func newPodCache(stopCh chan struct{}) *podCache {
	idxRules := []IdxFunc{
		generatePodIPKey,
		generateContainerIDKey,
		generateHostIPKey,
	}
	m := &podCache{}
	m.stopCh = stopCh
	m.eventCh = make(chan *K8sMetaEvent, 100)
	m.metaStore = NewDeferredDeletionMetaStore(m.eventCh, m.stopCh, 120, cache.MetaNamespaceKeyFunc, idxRules...)
	m.serviceMetaStore = NewDeferredDeletionMetaStore(m.eventCh, m.stopCh, 120, cache.MetaNamespaceKeyFunc)
	m.schema = runtime.NewScheme()
	_ = v1.AddToScheme(m.schema)
	return m
}

func (m *podCache) init(clientset *kubernetes.Clientset) {
	m.clientset = clientset
	m.metaStore.Start()
	m.watch(m.stopCh)
}

func (m *podCache) Get(key []string) map[string][]*ObjectWrapper {
	return m.metaStore.Get(key)
}

func (m *podCache) List() []*ObjectWrapper {
	return m.metaStore.List()
}

func (m *podCache) RegisterSendFunc(key string, sendFunc SendFunc, interval int) {
	m.metaStore.RegisterSendFunc(key, sendFunc, interval)
}

func (m *podCache) UnRegisterSendFunc(key string) {
	m.metaStore.UnRegisterSendFunc(key)
}

func generatePodIPKey(obj interface{}) ([]string, error) {
	pod, ok := obj.(*v1.Pod)
	if !ok {
		return []string{}, fmt.Errorf("object is not a pod")
	}
	return []string{pod.Status.PodIP}, nil
}

func generateContainerIDKey(obj interface{}) ([]string, error) {
	pod, ok := obj.(*v1.Pod)
	if !ok {
		return []string{}, fmt.Errorf("object is not a pod")
	}
	result := make([]string, len(pod.Status.ContainerStatuses))
	for i, containerStatus := range pod.Status.ContainerStatuses {
		result[i] = containerStatus.ContainerID
	}
	return result, nil
}

func generateHostIPKey(obj interface{}) ([]string, error) {
	pod, ok := obj.(*v1.Pod)
	if !ok {
		return []string{}, fmt.Errorf("object is not a pod")
	}
	return []string{pod.Status.HostIP}, nil
}

func (m *podCache) watch(stopCh <-chan struct{}) {
	factory := informers.NewSharedInformerFactory(m.clientset, time.Hour*24)
	informer := factory.Core().V1().Pods().Informer()
	informer.AddEventHandler(cache.ResourceEventHandlerFuncs{
		AddFunc: func(obj interface{}) {
			nowTime := time.Now().Unix()
			m.eventCh <- &K8sMetaEvent{
				EventType: EventTypeAdd,
				Object: &ObjectWrapper{
					ResourceType:      POD,
					Raw:               m.preProcess(obj),
					FirstObservedTime: nowTime,
					LastObservedTime:  nowTime,
				},
			}
		},
		UpdateFunc: func(oldObj interface{}, obj interface{}) {
			nowTime := time.Now().Unix()
			m.eventCh <- &K8sMetaEvent{
				EventType: EventTypeUpdate,
				Object: &ObjectWrapper{
					ResourceType:      POD,
					Raw:               m.preProcess(obj),
					FirstObservedTime: nowTime,
					LastObservedTime:  nowTime,
				},
			}
		},
		DeleteFunc: func(obj interface{}) {
			m.eventCh <- &K8sMetaEvent{
				EventType: EventTypeDelete,
				Object: &ObjectWrapper{
					ResourceType:     POD,
					Raw:              m.preProcess(obj),
					LastObservedTime: time.Now().Unix(),
				},
			}
		},
	})
	go factory.Start(stopCh)
	// wait infinite for first cache sync success
	for {
		if !cache.WaitForCacheSync(stopCh, informer.HasSynced) {
			logger.Error(context.Background(), "K8S_META_CACHE_SYNC_TIMEOUT", "pod cache sync timeout")
			time.Sleep(1 * time.Second)
		} else {
			break
		}
	}
}

func (m *podCache) preProcess(obj interface{}) interface{} {
	pod, ok := obj.(*v1.Pod)
	if !ok {
		return obj
	}
	pod.ManagedFields = nil
	pod.Status.Conditions = nil
	pod.Spec.Tolerations = nil
	if len(pod.Kind) == 0 {
		// Kind and API version may be empty because https://github.com/kubernetes/client-go/issues/541
		gvk, err := apiutil.GVKForObject(pod, m.schema)
		if err != nil {
			logger.Error(context.Background(), "K8S_META_CACHE_ALARM", "get GVK for object error", err)
			return pod
		}
		pod.APIVersion = gvk.GroupVersion().String()
		pod.Kind = gvk.Kind
	}
	return pod
}
