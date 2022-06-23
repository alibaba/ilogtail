package k8s_event

import (
	"github.com/golang/glog"
	corev1 "k8s.io/api/core/v1"
	v1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/runtime"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/kubernetes/scheme"
	typedcorev1 "k8s.io/client-go/kubernetes/typed/core/v1"
	restclient "k8s.io/client-go/rest"
	clientcmd "k8s.io/client-go/tools/clientcmd"
	"k8s.io/client-go/tools/record"
	ref "k8s.io/client-go/tools/reference"
)

type EventRecorder struct {
	recorder record.EventRecorder
	define   EventDefine
}

var eventRecorder = &EventRecorder{}

func SetEventRecorder(kubeclientset *kubernetes.Clientset, module string) {
	glog.V(4).Info("Creating event broadcaster")
	eventBroadcaster := record.NewBroadcaster()
	eventBroadcaster.StartLogging(glog.Infof)
	eventBroadcaster.StartRecordingToSink(&typedcorev1.EventSinkImpl{Interface: kubeclientset.CoreV1().Events("")})
	recorder := eventBroadcaster.NewRecorder(scheme.Scheme, corev1.EventSource{Component: Logtail})

	eventRecorder.recorder = recorder
	eventRecorder.define = *NewEventDefine(module)
}

func Init() {
	var cfg *restclient.Config

	//cfg, err := restclient.InClusterConfig()

	cfg, err := clientcmd.BuildConfigFromFlags("", "/root/.kube/config")
	if err != nil {
		//logger.Error("INIT_ALARM", "Error building kubeconfig: %s", err.Error())
		panic(nil)
	}
	kubeClient, err := kubernetes.NewForConfig(cfg)
	SetEventRecorder(kubeClient, "logtail")
}

func GetEventRecorder() *EventRecorder {
	return eventRecorder
}

func (e *EventRecorder) SendNormalEvent(object runtime.Object, action Action, message string) {
	if message == "" {
		message = "success"
	}
	e.recorder.Event(object, corev1.EventTypeNormal, e.define.getInfoAction(action), message)
}

func (e *EventRecorder) SendErrorEvent(object runtime.Object, action Action, alarm Alarm, message string) {
	if message == "" {
		message = "failed"
	}
	if alarm == "" {
		alarm = "Fail"
	}
	e.recorder.Event(object, corev1.EventTypeWarning, e.define.getErrorAction(action, alarm), message)
}

func (e *EventRecorder) SendNormalEventWithAnnotation(object runtime.Object, annotations map[string]string, action Action, message string) {
	if message == "" {
		message = "success"
	}
	e.recorder.AnnotatedEventf(object, annotations, corev1.EventTypeNormal, e.define.getInfoAction(action), message)
}

func (e *EventRecorder) SendErrorEventWithAnnotation(object runtime.Object, annotations map[string]string, action Action, alarm Alarm, message string) {
	if message == "" {
		message = "failed"
	}
	if alarm == "" {
		alarm = "Fail"
	}
	e.recorder.AnnotatedEventf(object, annotations, corev1.EventTypeWarning, e.define.getErrorAction(action, alarm), message)
}

func (e *EventRecorder) GetObject() runtime.Object {
	testPod := &v1.Pod{
		ObjectMeta: metav1.ObjectMeta{
			Name:      "logtail-ds",
			Namespace: "kube-system",
		},
	}
	ref, err := ref.GetReference(scheme.Scheme, testPod)
	if err == nil {
		return ref
	} else {
		return nil
	}
}
