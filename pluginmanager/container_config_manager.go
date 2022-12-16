// Copyright 2021 iLogtail Authors
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

package pluginmanager

import (
	"context"

	"time"

	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
)

// 24h
var FetchAllInterval = time.Second * time.Duration(24*60*60)

// 30min
var FirstFetchAllInterval = time.Second * time.Duration(30*60)

var timerFetchRunning = false

func TimerRecordData() {
	recordContainers(make(map[string]struct{}))
	// record all container confit result at same time
	util.RecordConfigResult()
}

// 记录增量的容器
func RecordAddContainers() {
	containerIDs := util.GetAddContainerIDs()
	logger.Info(context.Background(), "GetAddContainerIDs", containerIDs)
	if len(containerIDs) > 0 {
		// get project list
		recordContainers(containerIDs)
	}
}

func recordContainers(containerIDs map[string]struct{}) {
	envSet := make(map[string]struct{})
	labelSet := make(map[string]struct{})
	projectSet := make(map[string]struct{})
	for _, logstoreConfig := range LogtailConfig {
		for key := range logstoreConfig.EnvSet {
			envSet[key] = struct{}{}
		}
		for key := range logstoreConfig.LabelSet {
			labelSet[key] = struct{}{}
		}
		projectSet[logstoreConfig.ProjectName] = struct{}{}
	}
	keys := make([]string, 0, len(projectSet))
	for k := range projectSet {
		if len(k) > 0 {
			keys = append(keys, k)
		}
	}
	projectStr := util.GetStringFromList(keys)
	// get add container
	result := helper.GetAllContainerToRecord(envSet, labelSet, containerIDs)
	for _, containerInfo := range result {
		var containerDetailToRecord util.ContainerDetail
		containerDetailToRecord.Project = projectStr
		containerDetailToRecord.DataType = "all_container_info"
		containerDetailToRecord.ContainerID = containerInfo.Detail.ContainerInfo.ID
		containerDetailToRecord.ContainerIP = containerInfo.Detail.ContainerIP
		containerDetailToRecord.ContainerName = containerInfo.Detail.K8SInfo.ContainerName
		containerDetailToRecord.Driver = containerInfo.Detail.ContainerInfo.Driver
		containerDetailToRecord.HostsPath = containerInfo.Detail.ContainerInfo.HostsPath
		containerDetailToRecord.Hostname = containerInfo.Detail.ContainerInfo.Config.Hostname
		containerDetailToRecord.ImageName = containerInfo.Detail.ContainerInfo.Config.Image
		containerDetailToRecord.K8sLabels = containerInfo.Detail.K8SInfo.Labels
		containerDetailToRecord.Namespace = containerInfo.Detail.K8SInfo.Namespace
		containerDetailToRecord.PodName = containerInfo.Detail.K8SInfo.Pod
		containerDetailToRecord.LogPath = containerInfo.Detail.ContainerInfo.LogPath
		containerDetailToRecord.RootPath = containerInfo.Detail.DefaultRootPath
		containerDetailToRecord.RawContainerName = containerInfo.Detail.ContainerInfo.Name

		containerDetailToRecord.Env = containerInfo.Env
		containerDetailToRecord.Labels = containerInfo.Labels
		util.RecordContainer(&containerDetailToRecord)
	}
}

func CollectContainers(logGroup *protocol.LogGroup) {
	RecordAddContainers()
	util.SerializeContainerToPb(logGroup)
}

func CollectDeleteContainers(logGroup *protocol.LogGroup) {
	containerIDs := util.GetDeleteContainerIDs()
	logger.Info(context.Background(), "GetDeleteContainerIDs", containerIDs)
	if len(containerIDs) > 0 {
		projectSet := make(map[string]struct{})

		// get project list
		for _, logstoreConfig := range LogtailConfig {
			projectSet[logstoreConfig.ProjectName] = struct{}{}
		}
		keys := make([]string, len(projectSet))
		for k := range projectSet {
			if len(k) > 0 {
				keys = append(keys, k)
			}
		}
		projectStr := util.GetStringFromList(keys)

		ids := make([]string, len(containerIDs))
		for id := range containerIDs {
			if len(id) > 0 {
				ids = append(ids, id)
			}
		}
		logger.Info(context.Background(), "record GetDeleteContainerIDs", ids)
		util.SerializeDeleteContainerToPb(logGroup, projectStr, util.GetStringFromList(ids))
	}
}

func CollectConfigResult(logGroup *protocol.LogGroup) {
	util.SerializeConfigResultToPb(logGroup)
}

func timerFetchFuction() {
	timerFetch := func() {
		lastFetchAllTime := time.Now()
		// 第一次时间跟后面的周期性时间不同
		for {
			logger.Info(context.Background(), "First timerFetchFuction", time.Since(lastFetchAllTime))
			time.Sleep(time.Duration(10) * time.Second)
			if time.Since(lastFetchAllTime) >= FirstFetchAllInterval {
				logger.Info(context.Background(), "First PrintConfig fetch all", "start")
				TimerRecordData()
				lastFetchAllTime = time.Now()
				logger.Info(context.Background(), "First PrintConfig fetch all", "end")
				break
			}
		}
		for {
			logger.Info(context.Background(), "timerFetchFuction", time.Since(lastFetchAllTime))
			time.Sleep(time.Duration(10) * time.Second)
			if time.Since(lastFetchAllTime) >= FetchAllInterval {
				logger.Info(context.Background(), "PrintConfig fetch all", "start")
				TimerRecordData()
				lastFetchAllTime = time.Now()
				logger.Info(context.Background(), "PrintConfig fetch all", "end")
			}
		}
	}
	if !timerFetchRunning {
		go timerFetch()
		timerFetchRunning = true
	}
}
