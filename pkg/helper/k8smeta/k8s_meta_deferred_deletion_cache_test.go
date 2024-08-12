package k8smeta

import (
	"testing"
	"time"

	corev1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/client-go/tools/cache"
)

func TestDeferredDeletion(t *testing.T) {
	eventCh := make(chan *K8sMetaEvent)
	stopCh := make(chan struct{})
	gracePeriod := 1
	cache := NewDeferredDeletionMetaStore(eventCh, stopCh, int64(gracePeriod), cache.MetaNamespaceKeyFunc)
	cache.Start()
	pod := &corev1.Pod{
		ObjectMeta: metav1.ObjectMeta{
			Name:      "test",
			Namespace: "default",
		},
	}
	eventCh <- &K8sMetaEvent{
		EventType: EventTypeAdd,
		Object: &ObjectWrapper{
			Raw: pod,
		},
	}
	cache.lock.RLock()
	if _, ok := cache.Items["default/test"]; !ok {
		t.Errorf("failed to add object to cache")
	}
	cache.lock.RUnlock()
	eventCh <- &K8sMetaEvent{
		EventType: EventTypeDelete,
		Object: &ObjectWrapper{
			Raw: pod,
		},
	}
	cache.lock.RLock()
	if _, ok := cache.Items["default/test"]; !ok {
		t.Error("failed to deferred delete object from cache")
	}
	cache.lock.RUnlock()
	time.Sleep(time.Duration(gracePeriod+1) * time.Second)
	cache.lock.RLock()
	if _, ok := cache.Items["default/test"]; ok {
		t.Error("failed to delete object from cache")
	}
	cache.lock.RUnlock()
}
