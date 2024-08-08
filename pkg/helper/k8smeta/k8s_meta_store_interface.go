package k8smeta

//revive:disable:exported
type K8sMetaEvent struct {
	//revive:enable:exported
	EventType    string
	ResourceType string
	RawObject    interface{}
	CreateTime   int64
	UpdateTime   int64
}

type IdxFunc func(obj interface{}) ([]string, error)

type TimerHandler func(events []*K8sMetaEvent)
