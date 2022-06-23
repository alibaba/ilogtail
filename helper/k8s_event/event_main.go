package k8s_event

func main() {
	Init()
	GetEventRecorder().SendNormalEvent(nil, UpdateConfig, "123")
}
