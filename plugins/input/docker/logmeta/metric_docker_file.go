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

//go:build linux || windows
// +build linux windows

package logmeta

import (
	"encoding/json"
	"fmt"
	"os"
	"regexp"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/logtail"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/util"
)

const (
	PluginDockerUpdateFile    = 1
	PluginDockerDeleteFile    = 2
	PluginDockerUpdateFileAll = 3
	PluginDockerStopFile      = 4
)

type DockerFileUpdateCmd struct {
	ID   string
	Path string
	Tags []string
}

type DockerFileUpdateCmdAll struct {
	AllCmd []DockerFileUpdateCmd
}

type InputDockerFile struct {
	IncludeLabel          map[string]string // Deprecated： use IncludeContainerLabel and IncludeK8sLabel instead.
	ExcludeLabel          map[string]string // Deprecated： use ExcludeContainerLabel and ExcludeK8sLabel instead.
	IncludeEnv            map[string]string
	ExcludeEnv            map[string]string
	IncludeContainerLabel map[string]string
	ExcludeContainerLabel map[string]string
	IncludeK8sLabel       map[string]string
	ExcludeK8sLabel       map[string]string
	ExternalEnvTag        map[string]string
	ExternalK8sLabelTag   map[string]string
	LogPath               string
	FileParttern          string
	MountPath             string
	HostFlag              bool
	K8sNamespaceRegex     string
	K8sPodRegex           string
	K8sContainerRegex     string

	// export from ilogtail-trace component
	IncludeLabelRegex map[string]*regexp.Regexp
	ExcludeLabelRegex map[string]*regexp.Regexp
	IncludeEnvRegex   map[string]*regexp.Regexp
	ExcludeEnvRegex   map[string]*regexp.Regexp
	K8sFilter         *helper.K8SFilter

	FlushIntervalMs      int `comment:"the interval of container discovery, and the timeunit is millisecond. Default value is 3000."`
	lastPathMappingCache map[string]string
	context              pipeline.Context
	lastClearTime        time.Time
	updateEmptyFlag      bool
	avgInstanceMetric    pipeline.CounterMetric
	addMetric            pipeline.CounterMetric
	updateMetric         pipeline.CounterMetric
	deleteMetric         pipeline.CounterMetric
	lastUpdateTime       int64

	// Last return of GetAllAcceptedInfoV2
	fullList              map[string]bool
	matchList             map[string]*helper.DockerInfoDetail
	CollectContainersFlag bool
	firstStart            bool
}

func formatPath(path string) string {
	if len(path) == 0 {
		return path
	}
	if path[len(path)-1] == '/' {
		return path[0 : len(path)-1]
	}
	if path[len(path)-1] == '\\' {
		return path[0 : len(path)-1]
	}
	return path
}

func (idf *InputDockerFile) Name() string {
	return "InputDockerFile"
}

func (idf *InputDockerFile) Init(context pipeline.Context) (int, error) {
	idf.context = context
	idf.lastPathMappingCache = make(map[string]string)
	idf.firstStart = true
	idf.fullList = make(map[string]bool)
	idf.matchList = make(map[string]*helper.DockerInfoDetail)
	// Because docker on Windows will convert all mounted path to lowercase (see
	// Mounts field in output of docker inspect), so we have to change LogPath to
	// lowercase if it is a Windows path (with colon).
	idf.LogPath = formatPath(idf.LogPath)
	if colonPos := strings.Index(idf.LogPath, ":"); colonPos != -1 {
		idf.LogPath = strings.ToLower(idf.LogPath)
	}
	idf.lastClearTime = time.Now()
	if len(idf.LogPath) <= 1 {
		return 0, fmt.Errorf("empty log path")
	}
	helper.ContainerCenterInit()

	if idf.HostFlag {
		idf.MountPath = ""
	} else if envPath := os.Getenv("ALIYUN_LOGTAIL_MOUNT_PATH"); len(envPath) > 0 {
		if envPath[len(envPath)-1] == '/' || envPath[len(envPath)-1] == '\\' {
			envPath = envPath[0 : len(envPath)-1]
		}
		idf.MountPath = envPath
	} else {
		idf.MountPath = helper.DefaultLogtailMountPath
	}
	idf.updateEmptyFlag = true

	idf.avgInstanceMetric = helper.NewAverageMetric("container_count")
	idf.addMetric = helper.NewCounterMetric("add_container")
	idf.deleteMetric = helper.NewCounterMetric("remove_container")
	idf.updateMetric = helper.NewCounterMetric("update_container")
	idf.context.RegisterCounterMetric(idf.avgInstanceMetric)
	idf.context.RegisterCounterMetric(idf.addMetric)
	idf.context.RegisterCounterMetric(idf.deleteMetric)
	idf.context.RegisterCounterMetric(idf.updateMetric)

	var err error
	idf.IncludeEnv, idf.IncludeEnvRegex, err = helper.SplitRegexFromMap(idf.IncludeEnv)
	if err != nil {
		logger.Warning(idf.context.GetRuntimeContext(), "INVALID_REGEX_ALARM", "init include env regex error", err)
	}
	idf.ExcludeEnv, idf.ExcludeEnvRegex, err = helper.SplitRegexFromMap(idf.ExcludeEnv)
	if err != nil {
		logger.Warning(idf.context.GetRuntimeContext(), "INVALID_REGEX_ALARM", "init exclude env regex error", err)
	}
	if idf.IncludeLabel != nil {
		for k, v := range idf.IncludeContainerLabel {
			idf.IncludeLabel[k] = v
		}
	} else {
		idf.IncludeLabel = idf.IncludeContainerLabel
	}
	if idf.ExcludeLabel != nil {
		for k, v := range idf.ExcludeContainerLabel {
			idf.ExcludeLabel[k] = v
		}
	} else {
		idf.ExcludeLabel = idf.ExcludeContainerLabel
	}
	idf.IncludeLabel, idf.IncludeLabelRegex, err = helper.SplitRegexFromMap(idf.IncludeLabel)
	if err != nil {
		logger.Warning(idf.context.GetRuntimeContext(), "INVALID_REGEX_ALARM", "init include label regex error", err)
	}
	idf.ExcludeLabel, idf.ExcludeLabelRegex, err = helper.SplitRegexFromMap(idf.ExcludeLabel)
	if err != nil {
		logger.Warning(idf.context.GetRuntimeContext(), "INVALID_REGEX_ALARM", "init exclude label regex error", err)
	}
	idf.K8sFilter, err = helper.CreateK8SFilter(idf.K8sNamespaceRegex, idf.K8sPodRegex, idf.K8sContainerRegex, idf.IncludeK8sLabel, idf.ExcludeK8sLabel)

	logger.Debugf(idf.context.GetRuntimeContext(), "InputDockerFile inited successfully")
	return idf.FlushIntervalMs, err
}

func (idf *InputDockerFile) Description() string {
	return "docker file plugin for logtail"
}

func (idf *InputDockerFile) addMappingToLogtail(info *helper.DockerInfoDetail, destPath string, allCmd *DockerFileUpdateCmdAll) {
	var cmd DockerFileUpdateCmd
	cmd.ID = info.ContainerInfo.ID
	cmd.Path = destPath
	tags := info.GetExternalTags(idf.ExternalEnvTag, idf.ExternalK8sLabelTag)
	cmd.Tags = make([]string, 0, len(tags)*2)
	for key, val := range tags {
		cmd.Tags = append(cmd.Tags, key)
		cmd.Tags = append(cmd.Tags, val)
	}
	cmdBuf, _ := json.Marshal(&cmd)
	configName := idf.context.GetConfigName()
	logger.Info(idf.context.GetRuntimeContext(), "addMappingToLogtail cmd", cmd)
	if allCmd != nil {
		allCmd.AllCmd = append(allCmd.AllCmd, cmd)
		return
	}
	if err := logtail.ExecuteCMD(configName, PluginDockerUpdateFile, cmdBuf); err != nil {
		logger.Error(idf.context.GetRuntimeContext(), "DOCKER_FILE_MAPPING_ALARM", "cmdType", PluginDockerUpdateFile, "cmd", cmdBuf, "error", err)
	}
}

func (idf *InputDockerFile) deleteMappingFromLogtail(id string) {
	var cmd DockerFileUpdateCmd
	cmd.ID = id
	logger.Info(idf.context.GetRuntimeContext(), "deleteMappingFromLogtail cmd", cmd)
	cmdBuf, _ := json.Marshal(&cmd)
	configName := idf.context.GetConfigName()
	if err := logtail.ExecuteCMD(configName, PluginDockerDeleteFile, cmdBuf); err != nil {
		logger.Error(idf.context.GetRuntimeContext(), "DOCKER_FILE_MAPPING_ALARM", "cmdType", PluginDockerDeleteFile, "cmd", cmdBuf, "error", err)
	}
}

func (idf *InputDockerFile) notifyStopToLogtail(id string) {
	var cmd DockerFileUpdateCmd
	cmd.ID = id
	logger.Info(idf.context.GetRuntimeContext(), "notifyStopToLogtail cmd", cmd)
	cmdBuf, _ := json.Marshal(&cmd)
	configName := idf.context.GetConfigName()
	if err := logtail.ExecuteCMD(configName, PluginDockerStopFile, cmdBuf); err != nil {
		logger.Error(idf.context.GetRuntimeContext(), "DOCKER_FILE_MAPPING_ALARM", "cmdType", PluginDockerStopFile, "cmd", cmdBuf, "error", err)
	}
}

func (idf *InputDockerFile) updateAll(allCmd *DockerFileUpdateCmdAll) {
	logger.Info(idf.context.GetRuntimeContext(), "update all", len(allCmd.AllCmd))
	cmdBuf, _ := json.Marshal(allCmd)
	configName := idf.context.GetConfigName()
	if err := logtail.ExecuteCMD(configName, PluginDockerUpdateFileAll, cmdBuf); err != nil {
		logger.Error(idf.context.GetRuntimeContext(), "DOCKER_FILE_MAPPING_ALARM", "cmdType", PluginDockerUpdateFileAll, "cmd", cmdBuf, "error", err)
	}
}

func (idf *InputDockerFile) updateMapping(info *helper.DockerInfoDetail, sourcePath, containerPath string, allCmd *DockerFileUpdateCmdAll) {
	sourcePath = formatPath(sourcePath)
	containerPath = formatPath(containerPath)
	destPath := helper.GetMountedFilePathWithBasePath(idf.MountPath, sourcePath) + idf.LogPath[len(containerPath):]

	if val, ok := idf.lastPathMappingCache[info.ContainerInfo.ID]; ok && val != sourcePath {
		// send delete first and then add this info
		idf.updateMetric.Add(1)
		logger.Info(idf.context.GetRuntimeContext(), "container mapping", "changed", "last", val, "source host path", sourcePath, "destination container path", containerPath, "destination log path", destPath,
			"id", info.IDPrefix(), "name", info.ContainerInfo.Name, "created", info.ContainerInfo.Created, "status", info.Status())
		idf.lastPathMappingCache[info.ContainerInfo.ID] = sourcePath
		idf.addMappingToLogtail(info, destPath, allCmd)
	} else if !ok {
		idf.addMetric.Add(1)
		logger.Info(idf.context.GetRuntimeContext(), "container mapping", "added", "source host path", sourcePath, "destination container path", containerPath, "destination log path", destPath,
			"id", info.IDPrefix(), "name", info.ContainerInfo.Name, "created", info.ContainerInfo.Created, "status", info.Status())
		idf.lastPathMappingCache[info.ContainerInfo.ID] = sourcePath
		idf.addMappingToLogtail(info, destPath, allCmd)
	}
}

func (idf *InputDockerFile) deleteMapping(id string) {
	idf.deleteMappingFromLogtail(id)
	logger.Info(idf.context.GetRuntimeContext(), "container mapping", "deleted", "source path", idf.lastPathMappingCache[id], "id", helper.GetShortID(id))
	delete(idf.lastPathMappingCache, id)
}

func (idf *InputDockerFile) notifyStop(id string) {
	idf.notifyStopToLogtail(id)
	logger.Info(idf.context.GetRuntimeContext(), "container mapping", "stopped", "source path", idf.lastPathMappingCache[id], "id", helper.GetShortID(id))
}

func (idf *InputDockerFile) Collect(collector pipeline.Collector) error {
	newUpdateTime := helper.GetContainersLastUpdateTime()
	if idf.lastUpdateTime != 0 {
		// Nothing update, just skip.
		if idf.lastUpdateTime >= newUpdateTime {
			return nil
		}
	}

	var allCmd *DockerFileUpdateCmdAll
	allCmd = nil
	// if cache is empty, use update all cmd
	if len(idf.lastPathMappingCache) == 0 {
		allCmd = new(DockerFileUpdateCmdAll)
	}
	newCount, delCount, addResultList, deleteResultList := helper.GetContainerByAcceptedInfoV2(

		idf.fullList, idf.matchList,
		idf.IncludeLabel, idf.ExcludeLabel,
		idf.IncludeLabelRegex, idf.ExcludeLabelRegex,
		idf.IncludeEnv, idf.ExcludeEnv,
		idf.IncludeEnvRegex, idf.ExcludeEnvRegex,
		idf.K8sFilter)
	idf.lastUpdateTime = newUpdateTime
	// record config result
	havingPathkeys := make([]string, 0)
	nothavingPathkeys := make([]string, 0)
	if newCount != 0 || delCount != 0 {
		logger.Infof(idf.context.GetRuntimeContext(), "update match list, new: %v, delete: %v", newCount, delCount)
		// Can not return here because we should notify empty update to clear
		//  cache in docker_path_helper.json.
	} else {
		logger.Debugf(idf.context.GetRuntimeContext(), "update match list, new: %v, delete: %v", newCount, delCount)
	}

	dockerInfoDetails := idf.matchList
	logger.Debug(idf.context.GetRuntimeContext(), "match list length", len(dockerInfoDetails))
	idf.avgInstanceMetric.Add(int64(len(dockerInfoDetails)))

	for k, info := range dockerInfoDetails {
		sourcePath, containerPath := info.FindBestMatchedPath(idf.LogPath)

		formatSourcePath := formatPath(sourcePath)
		formateContainerPath := formatPath(containerPath)
		destPath := helper.GetMountedFilePathWithBasePath(idf.MountPath, formatSourcePath) + idf.LogPath[len(formateContainerPath):]

		if ok, err := util.PathExists(destPath); err == nil {
			if !ok {
				nothavingPathkeys = append(nothavingPathkeys, helper.GetShortID(k))
			} else {
				havingPathkeys = append(havingPathkeys, helper.GetShortID(k))
			}
		} else {
			nothavingPathkeys = append(nothavingPathkeys, helper.GetShortID(k))
			logger.Warning(idf.context.GetRuntimeContext(), "check docker mount path error", err.Error())
		}

		logger.Debugf(idf.context.GetRuntimeContext(), "bestMatchedPath for logPath:%v container id:%v name:%v created:%v status:%v sourcePath:%v containerPath:%v",
			idf.LogPath, info.ContainerInfo.ID, info.ContainerInfo.Name, info.ContainerInfo.Created, info.ContainerInfo.State.Status, sourcePath, containerPath)
		if len(sourcePath) > 0 {
			if info.ContainerInfo.State.Status == helper.ContainerStatusRunning {
				idf.updateMapping(info, sourcePath, containerPath, allCmd)
			}
		} else {
			logger.Warning(idf.context.GetRuntimeContext(), "DOCKER_FILE_MATCH_ALARM", "unknow error", "can't find path from this container", "path", idf.LogPath, "container", info.ContainerInfo.Name)
		}
	}
	if idf.CollectContainersFlag {
		configResult := &helper.ContainerConfigResult{
			DataType:                      "container_config_result",
			Project:                       idf.context.GetProject(),
			Logstore:                      idf.context.GetLogstore(),
			ConfigName:                    idf.context.GetConfigName(),
			SourceAddress:                 fmt.Sprintf("%s/**/%s", idf.LogPath, idf.FileParttern),
			PathExistInputContainerIDs:    helper.GetStringFromList(havingPathkeys),
			PathNotExistInputContainerIDs: helper.GetStringFromList(nothavingPathkeys),
			InputType:                     "file_log",
			InputIsContainerFile:          "true",
			FlusherType:                   "flusher_sls",
			FlusherTargetAddress:          fmt.Sprintf("%s/%s", idf.context.GetProject(), idf.context.GetLogstore()),
		}
		helper.RecordContainerConfigResultMap(configResult)
		if newCount != 0 || delCount != 0 || idf.firstStart {
			helper.RecordContainerConfigResultIncrement(configResult)
			idf.firstStart = false
		}
		logger.Debugf(idf.context.GetRuntimeContext(), "update match list, addResultList: %v, deleteResultList: %v", addResultList, deleteResultList)
	}

	for id := range idf.lastPathMappingCache {
		if c, ok := dockerInfoDetails[id]; !ok {
			idf.deleteMetric.Add(1)
			idf.notifyStop(id)
			idf.deleteMapping(id)
		} else if c.Status() != helper.ContainerStatusRunning {
			idf.notifyStop(id)
		}
	}
	if allCmd != nil {
		if len(allCmd.AllCmd) == 0 {
			// only update empty if empty flag is true
			if idf.updateEmptyFlag {
				idf.updateAll(allCmd)
				idf.updateEmptyFlag = false
			}
		} else {
			idf.updateAll(allCmd)
			idf.updateEmptyFlag = true
		}

	}

	if time.Since(idf.lastClearTime) > time.Hour {
		idf.lastPathMappingCache = make(map[string]string)
		idf.lastClearTime = time.Now()
	}

	return nil
}

func init() {
	pipeline.MetricInputs["metric_docker_file"] = func() pipeline.MetricInput {
		return &InputDockerFile{
			FlushIntervalMs: 3000,
		}
	}
}
