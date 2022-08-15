package jmxfetch

import (
	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/pkg/util"
	"path"
	"sync"
)

var once sync.Once
var manager *Manager

type Manager struct {
	configs          map[string]Instance
	loadedConfigs    map[string]Instance
	jmxFetchPath     string
	managerMeta      *helper.ManagerMeta
	jmxfetchdPath    string
	jmxfetchConfPath string
}

func (m *Manager) run() {

}

func GetJmxFetchManager(agentDirPath string) *Manager {
	once.Do(func() {
		manager = &Manager{
			configs:          make(map[string]Instance),
			loadedConfigs:    make(map[string]Instance),
			jmxFetchPath:     agentDirPath,
			managerMeta:      helper.NewmanagerMeta("jmxfetch"),
			jmxfetchdPath:    path.Join(agentDirPath, "telegrafd"),
			jmxfetchConfPath: path.Join(agentDirPath, "conf.d"),
		}
		// don't init the collector with struct because the collector depends on the bindMeta.
		util.RegisterAlarm("telegraf", manager.managerMeta.GetAlarm())
		go manager.run()
	})
	return manager
}
