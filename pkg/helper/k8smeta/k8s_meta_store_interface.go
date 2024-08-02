package k8smeta

type K8sMetaEvent struct {
	EventType    string
	ResourceType string
	RawObject    interface{}
	CreateTime   int64
	UpdateTime   int64
}

type IdxFunc func(obj interface{}) ([]string, error)

type TimerHandler func(events []*K8sMetaEvent)
