package jmxfetch

import (
	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"regexp"
	"sort"
	"strconv"
	"strings"
	"sync"
)

type Instance struct {
	Port int32
	Addr string
	Tags map[string]string
}

func (i *Instance) Hash() string {
	var hashStrBuilder strings.Builder
	hashStrBuilder.WriteString(string(i.Port))
	hashStrBuilder.WriteString(i.Addr)
	if len(i.Tags) > 0 {
		keys := make([]string, 0, len(i.Tags))
		for k := range i.Tags {
			keys = append(keys, k)
		}
		sort.Strings(keys)
		for idx := range keys {
			hashStrBuilder.WriteString(keys[idx])
			hashStrBuilder.WriteString(i.Tags[keys[idx]])
		}
	}
	return hashStrBuilder.String()
}

type Jmx struct {
	DiscoveryMode         bool // support container discovery
	IncludeEnv            map[string]string
	ExcludeEnv            map[string]string
	IncludeContainerLabel map[string]string
	ExcludeContainerLabel map[string]string
	IncludeK8sLabel       map[string]string
	ExcludeK8sLabel       map[string]string
	K8sNamespaceRegex     string
	K8sPodRegex           string
	K8sContainerRegex     string
	StaticInstances       []Instance
	JDKPath               string

	includeContainerLabelRegex map[string]*regexp.Regexp
	excludeContainerLabelRegex map[string]*regexp.Regexp
	includeEnvRegex            map[string]*regexp.Regexp
	excludeEnvRegex            map[string]*regexp.Regexp
	k8sFilter                  *helper.K8SFilter
	context                    ilogtail.Context
	instances                  map[string]Instance
	staticOnce                 sync.Once
}

func (m *Jmx) Init(context ilogtail.Context) (int, error) {
	m.context = context
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
	return 0, nil
}

func (m *Jmx) Description() string {
	return "a jmx fetch manger to generate configuration and control jmx fetch process(https://github.com/DataDog/jmxfetch)."
}

func (m *Jmx) Collect(collector ilogtail.Collector) error {
	if !m.DiscoveryMode {
		m.staticOnce.Do(func() {
			for i := range m.StaticInstances {
				m.instances[m.StaticInstances[i].Hash()] = m.StaticInstances[i]
			}
			// todo register configs
		})
		return nil
	}
	containers := helper.GetContainerByAcceptedInfo(m.IncludeContainerLabel, m.ExcludeContainerLabel, m.includeContainerLabelRegex, m.excludeContainerLabelRegex,
		m.IncludeEnv, m.ExcludeEnv, m.includeEnvRegex, m.excludeEnvRegex, m.k8sFilter)
	for _, detail := range containers {
		var port int32
		if val := detail.GetEnv("JMX_PORT"); val != "" {
			p, err := strconv.ParseInt(val, 10, 64)
			if err != nil {
				logger.Warning(m.context.GetRuntimeContext(), "ERROR_JMX_PORT", "the jmx port must be a number")
				continue
			}
			port = int32(p)
		}
		instance := Instance{
			Addr: detail.ContainerIP,
			Port: port,
			Tags: map[string]string{},
		}
		if detail.K8SInfo.ContainerName == "" {
			instance.Tags["container"] = detail.ContainerNameTag["_container_name_"]
		} else {
			instance.Tags["container"] = detail.K8SInfo.ContainerName
		}
		instance.Tags["namespace"] = detail.K8SInfo.Namespace
		instance.Tags["pod"] = detail.K8SInfo.Pod
		if val := detail.GetEnv("JMX_TAGS"); val != "" {
			parts := strings.Split(val, ",")
			for _, part := range parts {
				t := strings.Split(part, "=")
				if len(t) == 2 {
					instance.Tags[t[0]] = t[1]
				}
			}
		}
		m.instances[instance.Hash()] = instance
	}
	// todo register
	return nil
}

func init() {
	ilogtail.MetricInputs["metric_jmx"] = func() ilogtail.MetricInput {
		return &Jmx{
			DiscoveryMode: true,
			instances:     map[string]Instance{},
		}
	}
}
