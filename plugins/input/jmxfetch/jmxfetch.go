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
	"os"
	"path"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
	"time"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pluginmanager"
)

type Instance struct {
	Port     int32
	Host     string
	User     string
	Password string
	Tags     map[string]string
}
type Filter struct {
	Domain    string `yaml:"domain,omitempty"`
	BeanRegex string `yaml:"bean_regex,omitempty"`
	Type      string `yaml:"type,omitempty"`
	Attribute map[string]struct {
		MetricType string `yaml:"metric_type,omitempty"`
		Alias      string `yaml:"alias,omitempty"`
	} `yaml:"attribute,omitempty"`
}

type Jmx struct {
	// dynamic discovery
	DiscoveryMode         bool // support container discovery
	DiscoveryUser         string
	DiscoveryPassword     string
	IncludeEnv            map[string]string
	ExcludeEnv            map[string]string
	IncludeContainerLabel map[string]string
	ExcludeContainerLabel map[string]string
	IncludeK8sLabel       map[string]string
	ExcludeK8sLabel       map[string]string
	CollectK8sLabels      []string
	K8sNamespaceRegex     string
	K8sPodRegex           string
	K8sContainerRegex     string
	// static instances
	StaticInstances []*Instance
	// common config
	JDKPath                  string
	Filters                  []*Filter
	NewGcMetrics             bool
	CollectDefaultJvmMetrics bool

	includeContainerLabelRegex map[string]*regexp.Regexp
	excludeContainerLabelRegex map[string]*regexp.Regexp
	includeEnvRegex            map[string]*regexp.Regexp
	excludeEnvRegex            map[string]*regexp.Regexp
	k8sFilter                  *helper.K8SFilter
	context                    ilogtail.Context
	stopChan                   chan struct{}
	instances                  map[string]*InstanceInner
	key                        string // uniq key for binding collector
	jvmHome                    string
}

func (m *Jmx) Init(context ilogtail.Context) (int, error) {
	m.context = context
	m.key = m.context.GetProject() + m.context.GetLogstore() + m.context.GetConfigName()
	helper.ReplaceInvalidChars(&m.key)
	m.jvmHome = path.Join(pluginmanager.LogtailGlobalConfig.LogtailSysConfDir, "jvm")

	if m.JDKPath != "" {
		abs, err := filepath.Abs(filepath.Clean(m.JDKPath))
		if err != nil {
			logger.Error(m.context.GetRuntimeContext(), "PATH_ALARM", "the configured jdk path illegal", m.JDKPath)
			return 0, err
		}
		javaCmd := abs + "/bin/java"
		stat, err := os.Stat(javaCmd)
		// 73: 000 001 001 001
		if err != nil || stat.IsDir() || stat.Mode().Perm()&os.FileMode(73) == 0 {
			logger.Error(m.context.GetRuntimeContext(), "PATH_ALARM", "the configured jdk cmd path illegal", javaCmd)
			return 0, err
		}
	}
	GetJmxFetchManager(m.jvmHome).ConfigJavaHome(m.JDKPath)
	if m.DiscoveryMode {
		var err error
		m.IncludeEnv, m.includeEnvRegex, err = helper.SplitRegexFromMap(m.IncludeEnv)
		if err != nil {
			logger.Warning(m.context.GetRuntimeContext(), "INVALID_REGEX_ALARM", "init include env regex error", err)
		}
		m.ExcludeEnv, m.excludeEnvRegex, err = helper.SplitRegexFromMap(m.ExcludeEnv)
		if err != nil {
			logger.Warning(m.context.GetRuntimeContext(), "INVALID_REGEX_ALARM", "init exclude env regex error", err)
		}
		m.IncludeContainerLabel, m.includeContainerLabelRegex, err = helper.SplitRegexFromMap(m.IncludeContainerLabel)
		if err != nil {
			logger.Warning(m.context.GetRuntimeContext(), "INVALID_REGEX_ALARM", "init include label regex error", err)
		}
		m.ExcludeContainerLabel, m.excludeContainerLabelRegex, err = helper.SplitRegexFromMap(m.ExcludeContainerLabel)
		if err != nil {
			logger.Warning(m.context.GetRuntimeContext(), "INVALID_REGEX_ALARM", "init exclude label regex error", err)
		}
		m.k8sFilter, err = helper.CreateK8SFilter(m.K8sNamespaceRegex, m.K8sPodRegex, m.K8sContainerRegex, m.IncludeK8sLabel, m.ExcludeK8sLabel)
		if err != nil {
			logger.Warning(m.context.GetRuntimeContext(), "INVALID_REGEX_ALARM", "init k8s filter error", err)
		}
	}
	return 0, nil
}

func (m *Jmx) Description() string {
	return "a jmx fetch manger to generate configuration and control jmx fetch process(https://github.com/DataDog/jmxfetch)."
}

func (m *Jmx) Start(collector ilogtail.Collector) error {
	GetJmxFetchManager(m.jvmHome).RegisterCollector(m.key, collector, m.Filters)

	if !m.DiscoveryMode {
		logger.Infof(m.context.GetRuntimeContext(), "find %d static jmx configs", len(m.StaticInstances))
		for i := range m.StaticInstances {
			m.StaticInstances[i].Tags[dispatchKey] = m.key
			inner := NewInstanceInner(m.StaticInstances[i].Port, m.StaticInstances[i].Host, m.StaticInstances[i].User,
				m.StaticInstances[i].Password, m.StaticInstances[i].Tags, m.CollectDefaultJvmMetrics)
			m.instances[inner.Hash()] = inner
		}
		GetJmxFetchManager(m.jvmHome).Register(m.context, m.key, m.instances)
		return nil
	}
	go func() {
		ticker := time.NewTicker(time.Second * 5)
		for {
			select {
			case <-ticker.C:
				m.UpdateContainerCfg()
			case <-m.stopChan:
				ticker.Stop()
				return
			}
		}
	}()
	return nil
}

func (m *Jmx) Stop() error {
	logger.Infof(m.context.GetRuntimeContext(), "stopping")
	close(m.stopChan)
	GetJmxFetchManager(m.jvmHome).UnregisterCollector(m.key)
	return nil
}

func (m *Jmx) UpdateContainerCfg() {
	containers := helper.GetContainerByAcceptedInfo(m.IncludeContainerLabel, m.ExcludeContainerLabel,
		m.includeContainerLabelRegex, m.excludeContainerLabelRegex, m.IncludeEnv, m.ExcludeEnv,
		m.includeEnvRegex, m.excludeEnvRegex, m.k8sFilter)
	for _, detail := range containers {
		var port int32
		// use env because our k8s meta read from kubelet, labels maybe not correct.
		if val := detail.GetEnv("ILOGTAIL_JMX_PORT"); val != "" {
			p, err := strconv.ParseInt(val, 10, 64)
			if err != nil {
				logger.Warning(m.context.GetRuntimeContext(), "ERROR_JMX_PORT", "the jmx port must be a number")
				continue
			}
			port = int32(p)
		} else {
			continue
		}
		tags := make(map[string]string, 8)
		tags[dispatchKey] = m.key
		if detail.K8SInfo.ContainerName == "" {
			tags["container"] = detail.ContainerNameTag["_container_name_"]
		} else {
			tags["container"] = detail.K8SInfo.ContainerName
		}
		if detail.K8SInfo.Pod != "" {
			tags["namespace"] = detail.K8SInfo.Namespace
			tags["pod"] = detail.K8SInfo.Pod
		}

		for _, label := range m.CollectK8sLabels {
			val, ok := detail.K8SInfo.Labels[label]
			if ok {
				tags[label] = val
			}
		}

		if val := detail.GetEnv("JMX_TAGS"); val != "" {
			parts := strings.Split(val, ",")
			for _, part := range parts {
				t := strings.Split(part, "=")
				if len(t) == 2 {
					tags[t[0]] = t[1]
				}
			}
		}
		inner := NewInstanceInner(port, detail.ContainerIP, m.DiscoveryUser, m.DiscoveryPassword, tags, m.CollectDefaultJvmMetrics)
		m.instances[inner.Hash()] = inner
	}
	logger.Infof(m.context.GetRuntimeContext(), "find %d dynamic jmx configs", len(m.instances))
	GetJmxFetchManager(m.jvmHome).Register(m.context, m.key, m.instances)
}

func init() {
	ilogtail.ServiceInputs["service_jmx"] = func() ilogtail.ServiceInput {
		return &Jmx{
			DiscoveryMode:            false,
			NewGcMetrics:             true,
			CollectDefaultJvmMetrics: true,
			instances:                map[string]*InstanceInner{},
			stopChan:                 make(chan struct{}),
		}
	}
}
