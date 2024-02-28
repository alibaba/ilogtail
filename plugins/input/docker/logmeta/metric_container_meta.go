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
	"sort"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/logtail"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/util"
)

const (
	PluginContainerUpdateFile    = 1
	PluginContainerDeleteFile    = 2
	PluginContainerUpdateFileAll = 3
	PluginContainerStopFile      = 4
)

type ContainerFileUpdateCmd struct {
	// ID   string
	// Path string
	// Tags []string
	ID                string   // 容器ID
	Mounts            []string // Mounts信息
	ContainerInfoTags []string // 容器信息Tag
	UpperDir          string   // 容器UpperDir路径
	StdoutPath        string   // 标准输出路径
	StdoutLogType     string   // 标准输出类型 docker_json-file、containerd
}

type ContainerFileUpdateCmdAll struct {
	AllCmd []ContainerFileUpdateCmd
}

type InputContainerFile struct {
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
	FilePattern           string
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

	FlushIntervalMs   int `comment:"the interval of container discovery, and the timeunit is millisecond. Default value is 3000."`
	lastMountsCache   map[string][]string
	lastUpperDirCache map[string]string
	context           pipeline.Context
	lastClearTime     time.Time
	updateEmptyFlag   bool
	avgInstanceMetric pipeline.CounterMetric
	addMetric         pipeline.CounterMetric
	updateMetric      pipeline.CounterMetric
	deleteMetric      pipeline.CounterMetric
	lastUpdateTime    int64

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

func (idf *InputContainerFile) Name() string {
	return "InputContainerFile"
}

func (idf *InputContainerFile) Init(context pipeline.Context) (int, error) {
	idf.context = context
	idf.lastMountsCache = make(map[string][]string)
	idf.lastUpperDirCache = make(map[string]string)
	idf.firstStart = true
	idf.fullList = make(map[string]bool)
	idf.matchList = make(map[string]*helper.DockerInfoDetail)
	// Because docker on Windows will convert all mounted path to lowercase (see
	// Mounts field in output of container inspect), so we have to change LogPath to
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

	logger.Debugf(idf.context.GetRuntimeContext(), "InputContainerFile inited successfully")
	return idf.FlushIntervalMs, err
}

func (idf *InputContainerFile) Description() string {
	return "container file plugin for logtail"
}

// addMappingToLogtail  添加容器信息到allCmd里面，allCmd不为空时，只添加不执行，allCmd为空时，添加并执行
func (idf *InputContainerFile) addMappingToLogtail(info *helper.DockerInfoDetail, mounts []string, upperDir string, allCmd *ContainerFileUpdateCmdAll) {
	var cmd ContainerFileUpdateCmd
	cmd.ID = info.ContainerInfo.ID
	cmd.Mounts = mounts
	cmd.UpperDir = upperDir
	tags := info.GetExternalTags(idf.ExternalEnvTag, idf.ExternalK8sLabelTag)
	cmd.ContainerInfoTags = make([]string, 0, len(tags)*2)
	for key, val := range tags {
		cmd.ContainerInfoTags = append(cmd.ContainerInfoTags, key)
		cmd.ContainerInfoTags = append(cmd.ContainerInfoTags, val)
	}
	cmdBuf, _ := json.Marshal(&cmd)
	configName := idf.context.GetConfigName()
	logger.Info(idf.context.GetRuntimeContext(), "addMappingToLogtail cmd", cmd)
	if allCmd != nil {
		allCmd.AllCmd = append(allCmd.AllCmd, cmd)
		return
	}
	if err := logtail.ExecuteCMD(configName, PluginContainerUpdateFile, cmdBuf); err != nil {
		logger.Error(idf.context.GetRuntimeContext(), "DOCKER_FILE_MAPPING_ALARM", "cmdType", PluginContainerUpdateFile, "cmd", cmdBuf, "error", err)
	}
}

// updateAll  更新所有容器信息
func (idf *InputContainerFile) updateAll(allCmd *ContainerFileUpdateCmdAll) {
	logger.Info(idf.context.GetRuntimeContext(), "update all", len(allCmd.AllCmd))
	cmdBuf, _ := json.Marshal(allCmd)
	configName := idf.context.GetConfigName()
	if err := logtail.ExecuteCMD(configName, PluginContainerUpdateFileAll, cmdBuf); err != nil {
		logger.Error(idf.context.GetRuntimeContext(), "DOCKER_FILE_MAPPING_ALARM", "cmdType", PluginContainerUpdateFileAll, "cmd", cmdBuf, "error", err)
	}
}

func (idf *InputContainerFile) updateMapping(info *helper.DockerInfoDetail, mounts []string, upperDir string, allCmd *ContainerFileUpdateCmdAll) {
	// upperDir
	if val, ok := idf.lastUpperDirCache[info.ContainerInfo.ID]; ok && val != upperDir {
		// send delete first and then add this info
		idf.updateMetric.Add(1)
		logger.Info(idf.context.GetRuntimeContext(), "container UpperDir", "changed", "last", val, "now", upperDir,
			"id", info.IDPrefix(), "name", info.ContainerInfo.Name, "created", info.ContainerInfo.Created, "status", info.Status())
		idf.lastUpperDirCache[info.ContainerInfo.ID] = upperDir
		idf.addMappingToLogtail(info, mounts, upperDir, allCmd)
	} else if !ok {
		idf.addMetric.Add(1)
		logger.Info(idf.context.GetRuntimeContext(), "container UpperDir", "added", "upperDir", upperDir,
			"id", info.IDPrefix(), "name", info.ContainerInfo.Name, "created", info.ContainerInfo.Created, "status", info.Status())
		idf.lastUpperDirCache[info.ContainerInfo.ID] = upperDir
		idf.addMappingToLogtail(info, mounts, upperDir, allCmd)
	}
	// mounts
	sort.Strings(mounts)
	if val, ok := idf.lastMountsCache[info.ContainerInfo.ID]; ok {
		changed := false
		if len(val) == len(mounts) {
			for i := 0; i < len(mounts); i++ {
				if val[i] != mounts[i] {
					changed = true
					break
				}
			}
		} else {
			changed = true
		}
		if !changed {
			return
		}
		// send delete first and then add this info
		idf.updateMetric.Add(1)
		logger.Info(idf.context.GetRuntimeContext(), "container mounts", "changed", "last", val, "now", mounts,
			"id", info.IDPrefix(), "name", info.ContainerInfo.Name, "created", info.ContainerInfo.Created, "status", info.Status())
		idf.lastMountsCache[info.ContainerInfo.ID] = mounts
		idf.addMappingToLogtail(info, mounts, upperDir, allCmd)
	} else if !ok {
		idf.lastMountsCache[info.ContainerInfo.ID] = mounts
		idf.addMetric.Add(1)
		logger.Info(idf.context.GetRuntimeContext(), "container mounts", "added", "mounts", mounts,
			"id", info.IDPrefix(), "name", info.ContainerInfo.Name, "created", info.ContainerInfo.Created, "status", info.Status())
		idf.lastUpperDirCache[info.ContainerInfo.ID] = upperDir
		idf.addMappingToLogtail(info, mounts, upperDir, allCmd)
	}
}

// deleteMappingFromLogtail  把需要删除的容器信息的id 发送给c++
func (idf *InputContainerFile) deleteMappingFromLogtail(id string) {
	var cmd ContainerFileUpdateCmd
	cmd.ID = id
	logger.Info(idf.context.GetRuntimeContext(), "deleteMappingFromLogtail cmd", cmd)
	cmdBuf, _ := json.Marshal(&cmd)
	configName := idf.context.GetConfigName()
	if err := logtail.ExecuteCMD(configName, PluginContainerDeleteFile, cmdBuf); err != nil {
		logger.Error(idf.context.GetRuntimeContext(), "DOCKER_FILE_MAPPING_ALARM", "cmdType", PluginContainerDeleteFile, "cmd", cmdBuf, "error", err)
	}
}

// deleteMapping  删除容器信息
func (idf *InputContainerFile) deleteMapping(id string) {
	idf.deleteMappingFromLogtail(id)
	logger.Info(idf.context.GetRuntimeContext(), "container mapping", "deleted", "id", helper.GetShortID(id))
	delete(idf.lastMountsCache, id)
	delete(idf.lastUpperDirCache, id)
}

// notifyStopToLogtail 通知c++ 该容器已经停止
func (idf *InputContainerFile) notifyStopToLogtail(id string) {
	var cmd ContainerFileUpdateCmd
	cmd.ID = id
	logger.Info(idf.context.GetRuntimeContext(), "notifyStopToLogtail cmd", cmd)
	cmdBuf, _ := json.Marshal(&cmd)
	configName := idf.context.GetConfigName()
	if err := logtail.ExecuteCMD(configName, PluginContainerStopFile, cmdBuf); err != nil {
		logger.Error(idf.context.GetRuntimeContext(), "DOCKER_FILE_MAPPING_ALARM", "cmdType", PluginContainerStopFile, "cmd", cmdBuf, "error", err)
	}
}

// notifyStop 通知容器停止
func (idf *InputContainerFile) notifyStop(id string) {
	idf.notifyStopToLogtail(id)
	logger.Info(idf.context.GetRuntimeContext(), "container mapping", "stopped", "id", helper.GetShortID(id))
}

func (idf *InputContainerFile) Collect(collector pipeline.Collector) error {
	newUpdateTime := helper.GetContainersLastUpdateTime()
	if idf.lastUpdateTime != 0 {
		// Nothing update, just skip.
		if idf.lastUpdateTime >= newUpdateTime {
			return nil
		}
	}

	var allCmd *ContainerFileUpdateCmdAll
	allCmd = nil
	// if cache is empty, use update all cmd
	if len(idf.lastUpperDirCache) == 0 && len(idf.lastMountsCache) == 0 {
		allCmd = new(ContainerFileUpdateCmdAll)
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

	containerInfoDetails := idf.matchList
	logger.Debug(idf.context.GetRuntimeContext(), "match list length", len(containerInfoDetails))
	idf.avgInstanceMetric.Add(int64(len(containerInfoDetails)))

	for k, info := range containerInfoDetails {
		mounts := make([]string, 0, len(info.ContainerInfo.Mounts))
		for _, mount := range info.ContainerInfo.Mounts {
			formatMount := formatPath(mount.Source)
			mounts = append(mounts, formatMount)
		}
		upperDir := info.DefaultRootPath
		fmt.Println(mounts, upperDir)
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
			logger.Warning(idf.context.GetRuntimeContext(), "check container mount path error", err.Error())
		}

		logger.Debugf(idf.context.GetRuntimeContext(), "bestMatchedPath for logPath:%v container id:%v name:%v created:%v status:%v sourcePath:%v containerPath:%v",
			idf.LogPath, info.ContainerInfo.ID, info.ContainerInfo.Name, info.ContainerInfo.Created, info.ContainerInfo.State.Status, sourcePath, containerPath)
		if len(sourcePath) > 0 {
			if info.ContainerInfo.State.Status == helper.ContainerStatusRunning {
				idf.updateMapping(info, mounts, upperDir, allCmd)
			}
		} else {
			logger.Warning(idf.context.GetRuntimeContext(), "DOCKER_FILE_MATCH_ALARM", "unknow error", "can't find path from this container", "path", idf.LogPath, "container", info.ContainerInfo.Name)
		}
	}
	// 用于容器元信息预览
	if idf.CollectContainersFlag {
		configResult := &helper.ContainerConfigResult{
			DataType:                      "container_config_result",
			Project:                       idf.context.GetProject(),
			Logstore:                      idf.context.GetLogstore(),
			ConfigName:                    idf.context.GetConfigName(),
			SourceAddress:                 fmt.Sprintf("%s/**/%s", idf.LogPath, idf.FilePattern),
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

	for id := range idf.lastUpperDirCache {
		if c, ok := containerInfoDetails[id]; !ok {
			idf.deleteMetric.Add(1)
			idf.notifyStop(id)
			idf.deleteMapping(id)
		} else if c.Status() != helper.ContainerStatusRunning {
			idf.notifyStop(id)
		}
	}
	for id := range idf.lastMountsCache {
		if c, ok := containerInfoDetails[id]; !ok {
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
		idf.lastUpperDirCache = make(map[string]string)
		idf.lastMountsCache = make(map[string][]string)
		idf.lastClearTime = time.Now()
	}

	return nil
}

func init() {
	pipeline.MetricInputs["metric_container_meta"] = func() pipeline.MetricInput {
		return &InputContainerFile{
			FlushIntervalMs: 3000,
		}
	}
}
