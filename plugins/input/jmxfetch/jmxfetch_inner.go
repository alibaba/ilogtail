package jmxfetch

import (
	"sort"
	"strconv"
	"strings"
)

type InstanceInner struct {
	Port              int32    `yaml:"port,omitempty"`
	Host              string   `yaml:"host,omitempty"`
	User              string   `yaml:"user,omitempty"`
	Password          string   `yaml:"password,omitempty"`
	Tags              []string `yaml:"tags,omitempty"`
	DefaultJvmMetrics bool     `yaml:"collect_default_jvm_metrics"`
}

func (i *InstanceInner) Hash() string {
	var hashStrBuilder strings.Builder
	hashStrBuilder.WriteString(i.Host)
	hashStrBuilder.WriteString(strconv.Itoa(int(i.Port)))
	for idx := range i.Tags {
		hashStrBuilder.WriteString(i.Tags[idx])
	}
	return hashStrBuilder.String()
}

func NewInstanceInner(port int32, host, user, passowrd string, tags map[string]string, defaultJvmMetrics bool) *InstanceInner {
	tagsArr := make([]string, 0, len(tags))
	for k, v := range tags {
		tagsArr = append(tagsArr, k+":"+v)
	}
	sort.Strings(tagsArr)
	return &InstanceInner{
		Port:              port,
		Host:              host,
		User:              user,
		Password:          passowrd,
		Tags:              tagsArr,
		DefaultJvmMetrics: defaultJvmMetrics,
	}
}

type Cfg struct {
	include   []*Filter
	instances map[string]*InstanceInner
	change    bool
}

func NewCfg(filters []*Filter) *Cfg {
	return &Cfg{
		instances: map[string]*InstanceInner{},
		include:   filters,
	}
}
