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
	"sync"
	"time"

	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

// 12h
var FetchAllInterval = time.Second * time.Duration(12*60*60)

var timerFetchRunning = false

var envAndLabelMutex sync.Mutex

var envSet map[string]struct{}
var containerLabelSet map[string]struct{}
var k8sLabelSet map[string]struct{}

var cachedFullList map[string]struct{}
var containerMutex sync.Mutex

func timerRecordData() {
	containerMutex.Lock()
	cachedFullList = recordContainers(make(map[string]struct{}))
	containerMutex.Unlock()
	// record all container config result at same time
	helper.RecordContainerConfigResult()
}

func recordAddedContainers() {
	containerMutex.Lock()
	if cachedFullList == nil {
		cachedFullList = make(map[string]struct{})
	}
	fullAddedList, fullDeletedList := helper.GetDiffContainers(cachedFullList)
	containerMutex.Unlock()

	if len(fullDeletedList) > 0 {
		shortDeletedIDList := make([]string, 0)
		for _, containerID := range fullDeletedList {
			if len(containerID) > 0 {
				shortDeletedIDList = append(shortDeletedIDList, helper.GetShortID(containerID))
			}
		}
		helper.RecordDeletedContainerIDs(shortDeletedIDList)
		logger.Info(context.Background(), "fullDeletedList: ", fullDeletedList)
	}
	if len(fullAddedList) > 0 {
		containerIDs := make(map[string]struct{})
		for _, containerID := range fullAddedList {
			containerIDs[containerID] = struct{}{}
		}
		if len(containerIDs) > 0 {
			recordContainers(containerIDs)
		}
		logger.Info(context.Background(), "fullAddedList: ", fullAddedList)
	}
}

func recordContainers(containerIDs map[string]struct{}) map[string]struct{} {
	projectSet := make(map[string]struct{})
	recordedContainerIds := make(map[string]struct{})

	for _, logstoreConfig := range LogtailConfig {
		projectSet[logstoreConfig.ProjectName] = struct{}{}
	}
	keys := make([]string, 0, len(projectSet))
	for k := range projectSet {
		if len(k) > 0 {
			keys = append(keys, k)
		}
	}
	projectStr := helper.GetStringFromList(keys)
	// get add container
	envAndLabelMutex.Lock()
	result := helper.GetAllContainerToRecord(envSet, containerLabelSet, k8sLabelSet, containerIDs)
	envAndLabelMutex.Unlock()

	containerDetailToRecords := make([]*helper.ContainerDetail, 0)
	for _, containerInfo := range result {
		var containerDetailToRecord helper.ContainerDetail
		containerDetailToRecord.Project = projectStr
		containerDetailToRecord.DataType = "all_container_info"
		containerDetailToRecord.ContainerID = containerInfo.Detail.ContainerInfo.ID

		recordedContainerIds[containerInfo.Detail.ContainerInfo.ID] = struct{}{}

		containerDetailToRecord.ContainerIP = containerInfo.Detail.ContainerIP
		containerDetailToRecord.ContainerName = containerInfo.Detail.K8SInfo.ContainerName
		containerDetailToRecord.Driver = containerInfo.Detail.ContainerInfo.Driver
		containerDetailToRecord.HostsPath = containerInfo.Detail.ContainerInfo.HostsPath
		containerDetailToRecord.Hostname = containerInfo.Detail.ContainerInfo.Config.Hostname
		containerDetailToRecord.ImageName = containerInfo.Detail.ContainerInfo.Config.Image
		containerDetailToRecord.Namespace = containerInfo.Detail.K8SInfo.Namespace
		containerDetailToRecord.PodName = containerInfo.Detail.K8SInfo.Pod
		containerDetailToRecord.LogPath = containerInfo.Detail.ContainerInfo.LogPath
		containerDetailToRecord.RootPath = containerInfo.Detail.DefaultRootPath
		containerDetailToRecord.RawContainerName = containerInfo.Detail.ContainerInfo.Name

		containerDetailToRecord.Env = containerInfo.Env
		containerDetailToRecord.ContainerLabels = containerInfo.ContainerLabels
		containerDetailToRecord.K8sLabels = containerInfo.K8sLabels
		containerDetailToRecords = append(containerDetailToRecords, &containerDetailToRecord)
	}
	helper.RecordAddedContainers(containerDetailToRecords)
	return recordedContainerIds
}

func CollectContainers(logGroup *protocol.LogGroup) {
	recordAddedContainers()
	helper.SerializeContainerToPb(logGroup)
}

func CollectDeleteContainers(logGroup *protocol.LogGroup) {
	containerIDs := helper.GetDeletedContainerIDs()
	logger.Debugf(context.Background(), "GetDeletedContainerIDs", containerIDs)
	if len(containerIDs) > 0 {
		projectSet := make(map[string]struct{})

		// get project list
		for _, logstoreConfig := range LogtailConfig {
			projectSet[logstoreConfig.ProjectName] = struct{}{}
		}
		keys := make([]string, 0, len(projectSet))
		for k := range projectSet {
			if len(k) > 0 {
				keys = append(keys, k)
			}
		}
		projectStr := helper.GetStringFromList(keys)

		ids := make([]string, 0, len(containerIDs))
		for id := range containerIDs {
			if len(id) > 0 {
				ids = append(ids, id)
			}
		}
		helper.SerializeDeleteContainerToPb(logGroup, projectStr, helper.GetStringFromList(ids))
	}
}

func CollectConfigResult(logGroup *protocol.LogGroup) {
	helper.SerializeContainerConfigResultToPb(logGroup)
}

func refreshEnvAndLabel() {
	envAndLabelMutex.Lock()
	defer envAndLabelMutex.Unlock()
	envSet = make(map[string]struct{})
	containerLabelSet = make(map[string]struct{})
	k8sLabelSet = make(map[string]struct{})
	for _, logstoreConfig := range LogtailConfig {
		if logstoreConfig.CollectContainersFlag {
			for key := range logstoreConfig.EnvSet {
				envSet[key] = struct{}{}
			}
			for key := range logstoreConfig.ContainerLabelSet {
				containerLabelSet[key] = struct{}{}
			}
			for key := range logstoreConfig.K8sLabelSet {
				k8sLabelSet[key] = struct{}{}
			}
		}
	}
	logger.Debugf(context.Background(), "refreshEnvAndLabel", envSet, containerLabelSet, k8sLabelSet)
}

func compareEnvAndLabel() (diffEnvSet, diffContainerLabelSet, diffK8sLabelSet map[string]struct{}) {
	// get newest env label and compare with old
	diffEnvSet = make(map[string]struct{})
	diffContainerLabelSet = make(map[string]struct{})
	diffK8sLabelSet = make(map[string]struct{})
	for _, logstoreConfig := range LogtailConfig {
		if logstoreConfig.CollectContainersFlag {
			for key := range logstoreConfig.EnvSet {
				if _, ok := envSet[key]; !ok {
					envSet[key] = struct{}{}
					diffEnvSet[key] = struct{}{}
				}
			}
			for key := range logstoreConfig.ContainerLabelSet {
				if _, ok := containerLabelSet[key]; !ok {
					containerLabelSet[key] = struct{}{}
					diffContainerLabelSet[key] = struct{}{}
				}
			}
			for key := range logstoreConfig.K8sLabelSet {
				if _, ok := k8sLabelSet[key]; !ok {
					k8sLabelSet[key] = struct{}{}
					diffK8sLabelSet[key] = struct{}{}
				}
			}
		}
	}
	return diffEnvSet, diffContainerLabelSet, diffK8sLabelSet
}

func compareEnvAndLabelAndRecordContainer() {
	envAndLabelMutex.Lock()
	defer envAndLabelMutex.Unlock()

	diffEnvSet, diffContainerLabelSet, diffK8sLabelSet := compareEnvAndLabel()
	logger.Debugf(context.Background(), "compareEnvAndLabel", diffEnvSet, diffContainerLabelSet, diffK8sLabelSet)

	if len(diffEnvSet) != 0 || len(diffContainerLabelSet) != 0 || len(diffK8sLabelSet) != 0 {
		projectSet := make(map[string]struct{})
		for _, logstoreConfig := range LogtailConfig {
			projectSet[logstoreConfig.ProjectName] = struct{}{}
		}
		keys := make([]string, 0, len(projectSet))
		for k := range projectSet {
			if len(k) > 0 {
				keys = append(keys, k)
			}
		}
		projectStr := helper.GetStringFromList(keys)
		result := helper.GetAllContainerIncludeEnvAndLabelToRecord(envSet, containerLabelSet, k8sLabelSet, diffEnvSet, diffContainerLabelSet, diffK8sLabelSet)
		logger.Debugf(context.Background(), "GetAllContainerIncludeEnvAndLabelToRecord", result)

		containerDetailToRecords := make([]*helper.ContainerDetail, 0)

		for _, containerInfo := range result {
			var containerDetailToRecord helper.ContainerDetail
			containerDetailToRecord.Project = projectStr
			containerDetailToRecord.DataType = "all_container_info"
			containerDetailToRecord.ContainerID = containerInfo.Detail.ContainerInfo.ID
			containerDetailToRecord.ContainerIP = containerInfo.Detail.ContainerIP
			containerDetailToRecord.ContainerName = containerInfo.Detail.K8SInfo.ContainerName
			containerDetailToRecord.Driver = containerInfo.Detail.ContainerInfo.Driver
			containerDetailToRecord.HostsPath = containerInfo.Detail.ContainerInfo.HostsPath
			containerDetailToRecord.Hostname = containerInfo.Detail.ContainerInfo.Config.Hostname
			containerDetailToRecord.ImageName = containerInfo.Detail.ContainerInfo.Config.Image
			containerDetailToRecord.Namespace = containerInfo.Detail.K8SInfo.Namespace
			containerDetailToRecord.PodName = containerInfo.Detail.K8SInfo.Pod
			containerDetailToRecord.LogPath = containerInfo.Detail.ContainerInfo.LogPath
			containerDetailToRecord.RootPath = containerInfo.Detail.DefaultRootPath
			containerDetailToRecord.RawContainerName = containerInfo.Detail.ContainerInfo.Name
			containerDetailToRecord.Env = containerInfo.Env
			containerDetailToRecord.ContainerLabels = containerInfo.ContainerLabels
			containerDetailToRecord.K8sLabels = containerInfo.K8sLabels

			containerDetailToRecords = append(containerDetailToRecords, &containerDetailToRecord)
		}
		helper.RecordAddedContainers(containerDetailToRecords)
	}
}

func isCollectContainers() bool {
	for _, logstoreConfig := range LogtailConfig {
		if logstoreConfig.CollectContainersFlag {
			return true
		}
	}
	return false
}

func TimerFetchFuction() {
	timerFetch := func() {
		lastFetchAllTime := time.Now()
		refreshEnvAndLabel()
		for {
			logger.Debugf(context.Background(), "timerFetchFuction", time.Since(lastFetchAllTime))
			time.Sleep(time.Duration(10) * time.Second)
			if isCollectContainers() {
				fetchInterval := FetchAllInterval
				if time.Since(lastFetchAllTime) >= fetchInterval {
					logger.Info(context.Background(), "timerFetchFuction running", time.Since(lastFetchAllTime))
					refreshEnvAndLabel()
					timerRecordData()
					lastFetchAllTime = time.Now()

				} else {
					compareEnvAndLabelAndRecordContainer()
				}
			}
		}
	}
	if !timerFetchRunning {
		go timerFetch()
		timerFetchRunning = true
	}
}
