package k8smeta

import (
	"context"
	"fmt"
	"time"

	app "k8s.io/api/apps/v1"
	batch "k8s.io/api/batch/v1"
	v1 "k8s.io/api/core/v1"
	networking "k8s.io/api/networking/v1"
	storage "k8s.io/api/storage/v1"
	meta "k8s.io/apimachinery/pkg/api/meta"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/client-go/informers"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/tools/cache"
	"sigs.k8s.io/controller-runtime/pkg/client/apiutil"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type commonCache struct {
	metaStore *DeferredDeletionMetaStore
	clientset *kubernetes.Clientset

	eventCh chan *K8sMetaEvent
	stopCh  chan struct{}

	resourceType string
	schema       *runtime.Scheme
}

func newCommonCache(stopCh chan struct{}, resourceType string) *commonCache {
	idxRules := getIdxRules(resourceType)
	m := &commonCache{}
	m.eventCh = make(chan *K8sMetaEvent, 100)
	m.stopCh = stopCh
	m.metaStore = NewDeferredDeletionMetaStore(m.eventCh, m.stopCh, 120, cache.MetaNamespaceKeyFunc, idxRules...)
	m.resourceType = resourceType
	m.schema = runtime.NewScheme()
	v1.AddToScheme(m.schema)
	batch.AddToScheme(m.schema)
	app.AddToScheme(m.schema)
	networking.AddToScheme(m.schema)
	storage.AddToScheme(m.schema)
	return m
}

func (m *commonCache) init(clientset *kubernetes.Clientset) {
	m.clientset = clientset
	m.metaStore.Start()
	m.watch(m.stopCh)
}

func (m *commonCache) Get(key []string) map[string][]*ObjectWrapper {
	return m.metaStore.Get(key)
}

func (m *commonCache) List() []*ObjectWrapper {
	return m.metaStore.List()
}

func (m *commonCache) RegisterSendFunc(key string, sendFunc SendFunc, interval int) {
	m.metaStore.RegisterSendFunc(key, sendFunc, interval)
}

func (m *commonCache) UnRegisterSendFunc(key string) {
	m.metaStore.UnRegisterSendFunc(key)
}

func (m *commonCache) watch(stopCh <-chan struct{}) {
	factory := informers.NewSharedInformerFactory(m.clientset, time.Hour*1)
	informer := m.getInfromer(factory)
	if informer == nil {
		return
	}
	informer.AddEventHandler(cache.ResourceEventHandlerFuncs{
		AddFunc: func(obj interface{}) {
			nowTime := time.Now().Unix()
			m.eventCh <- &K8sMetaEvent{
				EventType: EventTypeAdd,
				Object: &ObjectWrapper{
					ResourceType:      m.resourceType,
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
					ResourceType:      m.resourceType,
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
					ResourceType:     m.resourceType,
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
			logger.Error(context.Background(), "K8S_META_CACHE_SYNC_TIMEOUT", "service cache sync timeout")
			time.Sleep(1 * time.Second)
		} else {
			break
		}
	}
}

func (m *commonCache) getInfromer(factory informers.SharedInformerFactory) cache.SharedIndexInformer {
	var informer cache.SharedIndexInformer
	switch m.resourceType {
	case SERVICE:
		informer = factory.Core().V1().Services().Informer()
	case DEPLOYMENT:
		informer = factory.Apps().V1().Deployments().Informer()
	case REPLICASET:
		informer = factory.Apps().V1().ReplicaSets().Informer()
	case STATEFULSET:
		informer = factory.Apps().V1().StatefulSets().Informer()
	case DAEMONSET:
		informer = factory.Apps().V1().DaemonSets().Informer()
	case CRONJOB:
		informer = factory.Batch().V1().CronJobs().Informer()
	case JOB:
		informer = factory.Batch().V1().Jobs().Informer()
	case NODE:
		informer = factory.Core().V1().Nodes().Informer()
	case NAMESPACE:
		informer = factory.Core().V1().Namespaces().Informer()
	case CONFIGMAP:
		informer = factory.Core().V1().ConfigMaps().Informer()
	case SECRET:
		informer = factory.Core().V1().Secrets().Informer()
	case PERSISTENTVOLUME:
		informer = factory.Core().V1().PersistentVolumes().Informer()
	case PERSISTENTVOLUMECLAIM:
		informer = factory.Core().V1().PersistentVolumeClaims().Informer()
	case STORAGECLASS:
		informer = factory.Storage().V1().StorageClasses().Informer()
	case INGRESS:
		informer = factory.Networking().V1().Ingresses().Informer()
	default:
		logger.Error(context.Background(), "ENTITY_PIPELINE_REGISTER_ERROR", "resourceType not support", m.resourceType)
		return nil
	}
	return informer
}

func getIdxRules(resourceType string) []IdxFunc {
	switch resourceType {
	case NODE:
		return []IdxFunc{generateNodeKey}
	default:
		return []IdxFunc{generateCommonKey}
	}
}

func (m *commonCache) preProcess(obj interface{}) interface{} {
	runtimeObj := obj.(runtime.Object)
	if runtimeObj.GetObjectKind().GroupVersionKind().Empty() {
		gvk, err := apiutil.GVKForObject(runtimeObj, m.schema)
		if err != nil {
			logger.Error(context.Background(), "K8S_META_PRE_PROCESS_ERROR", "get GVK for object error", err)
			return obj
		}
		runtimeObj.GetObjectKind().SetGroupVersionKind(gvk)
	}
	return runtimeObj
}

func generateCommonKey(obj interface{}) ([]string, error) {
	meta, err := meta.Accessor(obj)
	if err != nil {
		return []string{}, err
	}
	return []string{generateNameWithNamespaceKey(meta.GetNamespace(), meta.GetName())}, nil
}

func generateNodeKey(obj interface{}) ([]string, error) {
	node, err := meta.Accessor(obj)
	if err != nil {
		return []string{}, err
	}
	return []string{node.GetName()}, nil
}

func generateNameWithNamespaceKey(namespace, name string) string {
	return fmt.Sprintf("%s/%s", namespace, name)
}
