package service

import (
	"fmt"
	"testing"

	v1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"

	"github.com/alibaba/ilogtail/pkg/helper/k8smeta"
)

func TestProcessPodEntity(t *testing.T) {
	obj := &v1.Pod{
		ObjectMeta: metav1.ObjectMeta{
			Name:      "pod2",
			Namespace: "ns2",
			UID:       "uid2",
		},
		Spec: v1.PodSpec{
			Containers: []v1.Container{
				{
					Image: "k8s.gcr.io/hyperkube2_spec",
				},
				{
					Image: "k8s.gcr.io/hyperkube3_spec",
				},
			},
			InitContainers: []v1.Container{
				{
					Image: "k8s.gcr.io/initfoo_spec",
				},
			},
		},
		Status: v1.PodStatus{
			ContainerStatuses: []v1.ContainerStatus{
				{
					Name:        "container2",
					Image:       "k8s.gcr.io/hyperkube2",
					ImageID:     "docker://sha256:bbb",
					ContainerID: "docker://cd456",
				},
				{
					Name:        "container3",
					Image:       "k8s.gcr.io/hyperkube3",
					ImageID:     "docker://sha256:ccc",
					ContainerID: "docker://ef789",
				},
			},
			InitContainerStatuses: []v1.ContainerStatus{
				{
					Name:        "initContainer",
					Image:       "k8s.gcr.io/initfoo",
					ImageID:     "docker://sha256:wxyz",
					ContainerID: "docker://ef123",
				},
			},
		},
	}
	event := &k8smeta.K8sMetaEvent{
		RawObject: obj,
	}
	collector := &metaCollector{}
	log := collector.processPodEntity(event, "create")
	fmt.Println(log)
}
