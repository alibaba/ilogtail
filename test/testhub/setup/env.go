package setup

var Env TestEnv

type TestEnv interface {
	ExecOnLogtail(command string) error
	ExecOnSource(command string) error
}

func InitEnv(envType string) {
	switch envType {
	case "host":
		Env = NewHostEnv()
	case "daemonset":
		Env = NewDaemonSetEnv()
	}
}
