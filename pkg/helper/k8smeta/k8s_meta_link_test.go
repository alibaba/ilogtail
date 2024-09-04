package k8smeta

import (
	"testing"

	"github.com/stretchr/testify/assert"
	corev1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

func TestGetPodServiceLink(t *testing.T) {
	podCache := newPodCache(make(chan struct{}))
	serviceCache := newCommonCache(make(chan struct{}), SERVICE)
	serviceCache.metaStore.Items["default/test"] = &ObjectWrapper{
		Raw: &corev1.Service{
			ObjectMeta: metav1.ObjectMeta{
				Name:      "test",
				Namespace: "default",
			},
			Spec: corev1.ServiceSpec{
				Selector: map[string]string{
					"app": "test",
				},
			},
		},
	}
	serviceCache.metaStore.Items["default/test2"] = &ObjectWrapper{
		Raw: &corev1.Service{
			ObjectMeta: metav1.ObjectMeta{
				Name:      "test2",
				Namespace: "default",
			},
			Spec: corev1.ServiceSpec{
				Selector: map[string]string{
					"app": "test2",
				},
			},
		},
	}
	podCache.metaStore.Items["default/test"] = &ObjectWrapper{
		Raw: &corev1.Pod{
			ObjectMeta: metav1.ObjectMeta{
				Name:      "test",
				Namespace: "default",
				Labels: map[string]string{
					"app": "test",
					"env": "test",
				},
			},
			Spec: corev1.PodSpec{
				Containers: []corev1.Container{
					{
						Name:  "test",
						Image: "test",
					},
				},
			},
		},
	}
	podCache.metaStore.Items["default/test2"] = &ObjectWrapper{
		Raw: &corev1.Pod{
			ObjectMeta: metav1.ObjectMeta{
				Name:      "test2",
				Namespace: "default",
				Labels: map[string]string{
					"app": "test2",
					"env": "test2",
				},
			},
			Spec: corev1.PodSpec{
				Containers: []corev1.Container{
					{
						Name:  "test2",
						Image: "test2",
					},
				},
			},
		},
	}
	linkGenerator := NewK8sMetaLinkGenerator(map[string]MetaCache{
		POD:     podCache,
		SERVICE: serviceCache,
	})
	podList := []*K8sMetaEvent{
		{
			EventType: "update",
			Object:    podCache.metaStore.Items["default/test"],
		},
		{
			EventType: "update",
			Object:    podCache.metaStore.Items["default/test2"],
		},
	}
	results := linkGenerator.getPodServiceLink(podList)
	assert.Equal(t, 2, len(results))
	assert.Equal(t, "test", results[0].Object.Raw.(*PodService).Service.Name)
	assert.Equal(t, "test2", results[1].Object.Raw.(*PodService).Service.Name)
}
