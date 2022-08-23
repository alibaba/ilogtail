package agentmanager

import "github.com/alibaba/ilogtail/config_server/service/common"

type AgentManager struct {
	AgentMessageQueue common.Queue
}

const (
	opt_heartbeat string = "HEARTBEAT"
	opt_alarm     string = "ALARM"
	opt_status    string = "STATUS"
)

type agentMessage struct {
	Opt  string
	Data interface{}
}
