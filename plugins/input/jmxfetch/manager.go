// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package jmxfetch

import (
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path"
	"strconv"
	"sync"
	"time"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/pkg"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/util"
	"github.com/alibaba/ilogtail/plugins/input/udpserver"
	"github.com/alibaba/ilogtail/plugins/test/mock"

	"gopkg.in/yaml.v2"
)

var once sync.Once
var manager *Manager

const dispatchKey = "jmxfetch_ilogtail"

type CfgInstance struct {
	Host string   `json:"host" yaml:"host"`
	Port int      `json:"port" yaml:"port"`
	Name string   `json:"name" yaml:"name"`
	Tags []string `json:"tags" yaml:"tags"`
}

type Manager struct {
	loadedConfigs    map[string]*InstanceInner
	jmxFetchPath     string
	jmxfetchdPath    string
	jmxfetchConfPath string
	managerMeta      *helper.ManagerMeta
	server           *udpserver.SharedUDPServer

	stopChan     chan struct{}
	javaPath     string
	configChange bool
	sync.Mutex
	port int
}

func (m *Manager) RegisterCollector(key string, collector ilogtail.Collector) {
	m.server.RegisterCollectors(key, collector)
}

func (m *Manager) UnregisterCollector(key string) {
	m.server.UnregisterCollectors(key)
}

func (m *Manager) UnRegister(ctx ilogtail.Context, configs map[string]*InstanceInner) {
	m.Lock()
	defer m.Unlock()
	ltCtx, ok := ctx.GetRuntimeContext().Value(pkg.LogTailMeta).(*pkg.LogtailContextMeta)
	if ok {
		m.managerMeta.Delete(ltCtx.GetProject(), ltCtx.GetLogStore(), ltCtx.GetConfigName())
	}
	var todoDeleteCfgs bool
	for key := range m.loadedConfigs {
		_, ok := configs[key]
		if ok {
			todoDeleteCfgs = true
			delete(m.loadedConfigs, key)
		}
	}
	logger.Infof(m.managerMeta.GetContext(), "loaded config after unregister: %d", len(m.loadedConfigs))
	if len(m.loadedConfigs) == 0 {
		m.stopChan <- struct{}{}
	}
	m.configChange = m.configChange || todoDeleteCfgs
}

func (m *Manager) Register(ctx ilogtail.Context, configs map[string]*InstanceInner, javaPath string) {
	m.Lock()
	defer m.Unlock()
	ltCtx, ok := ctx.GetRuntimeContext().Value(pkg.LogTailMeta).(*pkg.LogtailContextMeta)
	if ok {
		m.managerMeta.Add(ltCtx.GetProject(), ltCtx.GetLogStore(), ltCtx.GetConfigName())
	}

	if m.javaPath == "" {
		p, err := m.checkJavaPath(javaPath)
		if err != nil {
			logger.Error(m.managerMeta.GetContext(), "JMXFETCH_ALARM", "java path", javaPath, "err", err)
			return
		}
		logger.Infof(m.managerMeta.GetContext(), "find jdk path: %s", p)
		err = m.installScripts(p)
		if err != nil {
			logger.Error(m.managerMeta.GetContext(), "JMXFETCH_ALARM", "jmxfetch script install fail", err)
			return
		}
		logger.Info(m.managerMeta.GetContext(), "install jmx scripts success")
		m.javaPath = p
	}

	var todoAddCfgs, todoDeleteCfgs bool
	for key := range m.loadedConfigs {
		_, ok := configs[key]
		if !ok {
			todoDeleteCfgs = true
			delete(m.loadedConfigs, key)
		}
	}
	for key := range configs {
		_, ok := m.loadedConfigs[key]
		if !ok {
			todoAddCfgs = true
			m.loadedConfigs[key] = configs[key]
		}
	}
	logger.Infof(m.managerMeta.GetContext(), "loaded config after register: %d", len(m.loadedConfigs))
	m.configChange = m.configChange || todoDeleteCfgs || todoAddCfgs
}

func (m *Manager) run() {
	cfgticker := time.NewTicker(time.Second * 5)
	statusticker := time.NewTicker(time.Second * 5)

	updateFunc := func() {
		m.Lock()
		defer m.Unlock()
		if m.configChange {
			m.updateFiles()
			m.configChange = false
		}
	}
	startFunc := func() {
		m.Lock()
		defer m.Unlock()
		if len(m.loadedConfigs) > 0 {
			if !m.server.IsRunning() {
				if err := m.server.Start(); err != nil {
					logger.Error(m.managerMeta.GetContext(), "JMX_ALARM", "start jmx server err", err)
				}
			}
			m.start()
		}
	}

	stopFunc := func() {
		m.Lock()
		defer m.Unlock()
		if len(m.loadedConfigs) == 0 {
			if m.server.IsRunning() {
				_ = m.server.Stop()
			}
			m.stop()
			m.javaPath = ""
		}
	}

	for {
		select {
		case <-cfgticker.C:
			updateFunc()
		case <-statusticker.C:
			startFunc()
		case <-m.stopChan:
			stopFunc()
		}
	}
}

func (m *Manager) updateFiles() {
	if len(m.loadedConfigs) == 0 {
		return
	}
	cfg := make(map[string]interface{})
	cfg["init_config"] = map[string]interface{}{
		"is_jmx":                  true,
		"collect_default_metrics": true,
	}
	var instances []*InstanceInner
	for k := range m.loadedConfigs {
		instances = append(instances, m.loadedConfigs[k])
	}
	cfg["instances"] = instances
	bytes, err := yaml.Marshal(cfg)
	if err != nil {
		logger.Error(m.managerMeta.GetContext(), "JMXFETCH_CONFIG_ALARM", "cannot convert to yaml bytes", err)
		return
	}
	cfgPath := path.Join(m.jmxfetchConfPath, "config.yaml")
	logger.Debug(m.managerMeta.GetContext(), "write files", string(bytes), "path", cfgPath)
	err = ioutil.WriteFile(cfgPath, bytes, 0600)
	if err != nil {
		logger.Error(m.managerMeta.GetContext(), "JMXFETCH_CONFIG_ALARM", "write config file err", err, "path", cfgPath)
	}
}

func (m *Manager) checkJavaPath(javaPath string) (string, error) {
	if javaPath == "" {
		cmd := exec.Command("which", "java")
		bytes, err := util.CombinedOutputTimeout(cmd, time.Second)
		logger.Debugf(m.managerMeta.GetContext(), "detect java path: %s", string(bytes))
		if err != nil {
			return "", fmt.Errorf("java path is illegal: %v", err)
		}
		// remove \n
		javaPath = string(bytes[:len(bytes)-1])
	}
	cmd := exec.Command(javaPath, "-version") //nolint:gosec
	if _, err := util.CombinedOutputTimeout(cmd, time.Second); err != nil {
		return "", fmt.Errorf("java cmd is illegal: %v", err)
	}
	return javaPath, nil
}

func (m *Manager) initAgentDir() {
	logger.Infof(m.managerMeta.GetContext(), "init jmxfetch path: %s", m.jmxFetchPath)
	if exist, err := util.PathExists(m.jmxFetchPath); !exist {
		if err != nil {
			logger.Warningf(m.managerMeta.GetContext(), "JMXFETCH_ALARM", "create conf dir error, path %v, err: %v", m.jmxFetchPath, err)
			return
		}
		err = os.MkdirAll(m.jmxFetchPath, 0750)
		if err != nil {
			logger.Warningf(m.managerMeta.GetContext(), "JMXFETCH_ALARM", "create conf dir error, path %v, err: %v", m.jmxFetchPath, err)
		}
	}
	if exist, err := util.PathExists(m.jmxfetchConfPath); !exist {
		if err != nil {
			logger.Warningf(m.managerMeta.GetContext(), "JMXFETCH_ALARM", "create conf dir error, path %v, err: %v", m.jmxfetchConfPath, err)
			return
		}
		err = os.MkdirAll(m.jmxfetchConfPath, 0750)
		if err != nil {
			logger.Warningf(m.managerMeta.GetContext(), "JMXFETCH_ALARM", "create conf dir error, path %v, err: %v", m.jmxfetchConfPath, err)
		}
	} else {
		// Clean config files (outdated) in conf directory.
		if files, err := ioutil.ReadDir(m.jmxfetchConfPath); err == nil {
			for _, f := range files {
				filePath := path.Join(m.jmxfetchConfPath, f.Name())
				if err = os.Remove(filePath); err == nil {
					logger.Infof(m.managerMeta.GetContext(), "delete outdated agent config file: %v", filePath)
				} else {
					logger.Warningf(m.managerMeta.GetContext(), "deleted outdated agent config file err, path: %v, err: %v",
						filePath, err)
				}
			}
		} else {
			logger.Warningf(m.managerMeta.GetContext(), "JMXFETCH_ALARM",
				"clean conf dir error, path %v, err: %v", m.jmxfetchConfPath, err)
		}
	}
}

func GetJmxFetchManager(agentDirPath string) *Manager {
	once.Do(func() {
		manager = &Manager{
			loadedConfigs:    make(map[string]*InstanceInner),
			managerMeta:      helper.NewmanagerMeta("jmxfetch"),
			jmxFetchPath:     agentDirPath,
			jmxfetchConfPath: path.Join(agentDirPath, "conf.d"),
			jmxfetchdPath:    path.Join(agentDirPath, scriptsName),
			stopChan:         make(chan struct{}),
		}
		// don't init the collector with struct because the collector depends on the bindMeta.
		util.RegisterAlarm("jmxfetch", manager.managerMeta.GetAlarm())
		manager.initAgentDir()

		manager.port, _ = helper.GetFreePort()
		manager.server, _ = udpserver.NewSharedUDPServer(mock.NewEmptyContext("", "", "jmxfetchserver"), "statsd", ":"+strconv.Itoa(manager.port), dispatchKey, 65535)
		go manager.run()
	})
	return manager
}
