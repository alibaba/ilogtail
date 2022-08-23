package configmanager

import "github.com/alibaba/ilogtail/config_server/service/model"

type ConfigManager struct {
	ConfigList map[string]*model.Config
}
