package goprofile

import (
	"context"
	"net"
	"regexp"
	"strconv"

	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/pyroscope-io/pyroscope/pkg/scrape/discovery"
	"github.com/pyroscope-io/pyroscope/pkg/scrape/discovery/targetgroup"
	"github.com/pyroscope-io/pyroscope/pkg/scrape/model"
	"time"
)

type KubernetesConfig struct {
	IncludeEnv            map[string]string
	ExcludeEnv            map[string]string
	IncludeContainerLabel map[string]string
	ExcludeContainerLabel map[string]string
	IncludeK8sLabel       map[string]string
	ExcludeK8sLabel       map[string]string
	ExternalEnvTag        map[string]string
	ExternalK8sLabelTag   map[string]string
	K8sNamespaceRegex     string
	K8sPodRegex           string
	K8sContainerRegex     string

	IncludeContainerLabelRegex map[string]*regexp.Regexp
	ExcludeContainerLabelRegex map[string]*regexp.Regexp
	IncludeEnvRegex            map[string]*regexp.Regexp
	ExcludeEnvRegex            map[string]*regexp.Regexp
	K8sFilter                  *helper.K8SFilter

	// Last return of GetAllAcceptedInfoV2
	fullList      map[string]bool
	matchList     map[string]*helper.DockerInfoDetail
	lastClearTime time.Time
	labelSet      model.LabelSet
}

func (k *KubernetesConfig) Name() string {
	return "kubernetes_ilogtail"
}

func (k *KubernetesConfig) NewDiscoverer(options discovery.DiscovererOptions) (discovery.Discoverer, error) {
	k.InitDiscoverer()
	return k, nil
}

func (k *KubernetesConfig) InitDiscoverer() {
	k.fullList = make(map[string]bool)
	k.matchList = make(map[string]*helper.DockerInfoDetail)
	k.lastClearTime = time.Now()
	helper.ContainerCenterInit()
	var err error
	k.IncludeEnv, k.IncludeEnvRegex, err = helper.SplitRegexFromMap(k.IncludeEnv)
	if err != nil {
		logger.Warning(context.Background(), "INVALID_REGEX_ALARM", "init include env regex error", err)
	}
	k.ExcludeEnv, k.ExcludeEnvRegex, err = helper.SplitRegexFromMap(k.ExcludeEnv)
	if err != nil {
		logger.Warning(context.Background(), "INVALID_REGEX_ALARM", "init exclude env regex error", err)
	}
	k.IncludeContainerLabel, k.IncludeContainerLabelRegex, err = helper.SplitRegexFromMap(k.IncludeContainerLabel)
	if err != nil {
		logger.Warning(context.Background(), "INVALID_REGEX_ALARM", "init include label regex error", err)
	}
	k.ExcludeContainerLabel, k.ExcludeContainerLabelRegex, err = helper.SplitRegexFromMap(k.ExcludeContainerLabel)
	if err != nil {
		logger.Warning(context.Background(), "INVALID_REGEX_ALARM", "init exclude label regex error", err)
	}
	k.K8sFilter, err = helper.CreateK8SFilter(k.K8sNamespaceRegex, k.K8sPodRegex, k.K8sContainerRegex, k.IncludeK8sLabel, k.ExcludeK8sLabel)

	logger.Debug(context.Background(), "profile_go_kubernetes inited successfully")
}

func (k *KubernetesConfig) Run(ctx context.Context, up chan<- []*targetgroup.Group) {
	defer func() {
		logger.Info(ctx, "stop ilogtail kubernetes discovery")
	}()
	ticker := time.NewTicker(time.Second)
	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			containers := helper.GetContainerByAcceptedInfo(k.IncludeContainerLabel, k.ExcludeContainerLabel,
				k.IncludeContainerLabelRegex, k.ExcludeContainerLabelRegex, k.IncludeEnv, k.ExcludeEnv,
				k.IncludeEnvRegex, k.ExcludeEnvRegex, k.K8sFilter)
			up <- k.convertContainers2Group(containers)
		}
	}
}

func (k *KubernetesConfig) convertContainers2Group(containers map[string]*helper.DockerInfoDetail) []*targetgroup.Group {
	res := make([]*targetgroup.Group, 0, len(containers))

	for id, detail := range containers {
		val := detail.GetEnv("ILOGTAIL_PROFILE_PORT")
		if val == "" {
			continue
		}
		if _, err := strconv.Atoi(val); err != nil {
			logger.Debug(context.Background(), "ignore container because invalid ", detail.PodName())
			continue
		}
		var g targetgroup.Group
		addr := net.JoinHostPort(detail.ContainerIP, val)
		target := model.LabelSet{
			model.AddressLabel: model.LabelValue(addr),
		}
		tags := make(map[string]string)
		detail.GetCustomExternalTags(tags, k.ExternalEnvTag, k.ExternalK8sLabelTag)
		for k, v := range tags {
			target[model.LabelName(k)] = model.LabelValue(v)
		}
		if detail.K8SInfo.Pod != "" {
			if detail.K8SInfo.ContainerName == "" {
				target["container"] = model.LabelValue(detail.ContainerNameTag["_container_name_"])
			} else {
				target["container"] = model.LabelValue(detail.K8SInfo.ContainerName)
			}
			if detail.K8SInfo.Pod != "" {
				target["namespace"] = model.LabelValue(detail.K8SInfo.Namespace)
				target["pod"] = model.LabelValue(detail.K8SInfo.Pod)
				target["service"] = model.LabelValue(helper.ExtractPodWorkload(detail.K8SInfo.Pod))
			}
		}
		g.Targets = append(g.Targets, target)
		g.Labels = k.labelSet
		g.Source = id
	}
	return res
}
