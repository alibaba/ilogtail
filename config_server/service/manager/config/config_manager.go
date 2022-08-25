package configmanager

import "github.com/alibaba/ilogtail/config_server/service/model"

type ConfigManager struct {
	ConfigList map[string]*model.Config
}

// for agent checking config list
type CheckResult struct {
	ConfigName string       `json:"config_name"`
	Msg        string       `json:"message"`
	Data       model.Config `json:"detail"`
}
