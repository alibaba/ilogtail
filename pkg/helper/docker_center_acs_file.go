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

package helper

import (
	"bufio"
	"encoding/json"
	"io/fs"
	"os"
	"path/filepath"
	"strings"
	"time"

	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"

	"github.com/alibaba/ilogtail/pkg/util"
)

const acsFlag = "ALIYUN_LOG_ACS"
const acsStaticContainerInfoMountPathEnvKey = "ALIYUN_LOG_ACS_STATIC_CONTAINER_INFO_MOUNT_PATH"
const acsPodInfoMountPathEnvKey = "ALIYUN_LOG_ACS_POD_INFO_MOUNT_PATH"

const containerNameAnnotationKey = "io.kubernetes.cri.container-name"
const imageNameAnnotationKey = "io.kubernetes.cri.image-name"
const containerTypeAnnotationKey = "io.kubernetes.cri.container-type"

const containerNameLabelKey = "io.kubernetes.container.name"
const podNameLabelKey = "io.kubernetes.pod.name"
const namespaceLabelKey = "io.kubernetes.pod.namespace"

const defaultAcsStaticContainerInfoMountPath = "/logtail_host/bundle"
const defaultAcsPodInfoMountPath = "/etc/podinfo"

// adhoc
const defaultStdoutMountPath = "/../stdlog"

var acsStaticContainerInfoMountPath string
var acsPodInfoMountPath string

var lastContainerName2Info map[string]string

var staticContainers []types.ContainerJSON

type OCIContainerInfo struct {
	Annotations map[string]string
	Process     OCIContainerProcessInfo
	Mounts      []staticContainerMount
	State       staticContainerState
}

type OCIContainerProcessInfo struct {
	Env []string
}

func staticACSContainerInfoToStandard(staticInfo *OCIContainerInfo, stat fs.FileInfo, containerID string) types.ContainerJSON {
	created := stat.ModTime()

	allEnv := staticInfo.Process.Env

	hostname := util.GetHostName()

	containerIP := util.GetIPAddress()

	status := staticInfo.State.Status
	if status != ContainerStatusExited {
		if !ContainerProcessAlive(staticInfo.State.Pid) {
			status = ContainerStatusExited
		}
	}

	containerName := staticInfo.Annotations[containerNameAnnotationKey]
	imageName := staticInfo.Annotations[imageNameAnnotationKey]

	namespace, podName, podLabels := getPodMeta()
	fillPodLabels(namespace, podName, containerName, podLabels)

	mockUpperDir := make(map[string]string)

	// todo
	mockUpperDir["UpperDir"] = "/bundle/" + containerID + "/rootfs"

	dockerContainer := types.ContainerJSON{
		ContainerJSONBase: &types.ContainerJSONBase{
			ID:      containerID,
			Name:    containerName,
			Created: created.Format(time.RFC3339Nano),
			LogPath: defaultStdoutMountPath + "/" + containerName + "/0.log",

			State: &types.ContainerState{
				// todo
				Status: "running",
				Pid:    staticInfo.State.Pid,
			},
			GraphDriver: types.GraphDriverData{
				Data: mockUpperDir,
			},
		},
		Config: &container.Config{
			Labels:   podLabels,
			Image:    imageName,
			Env:      allEnv,
			Hostname: hostname,
		},
		NetworkSettings: &types.NetworkSettings{
			DefaultNetworkSettings: types.DefaultNetworkSettings{
				IPAddress: containerIP,
			},
		},
	}

	for _, mount := range staticInfo.Mounts {
		dockerContainer.Mounts = append(dockerContainer.Mounts, types.MountPoint{
			Source:      mount.Source,
			Destination: mount.Destination,
			Driver:      mount.Driver,
		})
	}
	return dockerContainer
}

func getPodMeta() (string, string, map[string]string) {

	envPath := os.Getenv(acsPodInfoMountPathEnvKey)
	if len(envPath) == 0 {
		acsPodInfoMountPath = defaultAcsPodInfoMountPath
	} else {
		acsPodInfoMountPath = envPath
	}

	namespaceFile := acsPodInfoMountPath + "/namespace"
	podnameFile := acsPodInfoMountPath + "/name"
	podLabelFile := acsPodInfoMountPath + "/labels"

	namespace := ""
	podName := ""
	podLabels := make(map[string]string)

	namespaceBody, err := os.ReadFile(filepath.Clean(namespaceFile))
	if err == nil {
		namespace = string(namespaceBody)
	}
	podNameBody, err := os.ReadFile(filepath.Clean(podnameFile))
	if err == nil {
		podName = string(podNameBody)
	}

	file, err := os.Open(podLabelFile)
	if err != nil {
		return namespace, podName, podLabels
	}
	defer file.Close() // 确保在函数结束时关闭文件

	// 使用 bufio.NewScanner 创建 Scanner 对象
	scanner := bufio.NewScanner(file)

	// 使用 Scan 方法逐行读取文件
	for scanner.Scan() {
		line := scanner.Text() // 获取当前行的内容
		parts := strings.Split(line, "=")
		if len(parts) == 2 {
			key := parts[0]
			value := strings.Trim(parts[1], "\"")
			podLabels[key] = value
		}
	}
	return namespace, podName, podLabels
}

func fillPodLabels(namespace, podName, containerName string, podLabels map[string]string) {
	podLabels[namespaceLabelKey] = namespace
	podLabels[podNameLabelKey] = podName
	podLabels[containerNameLabelKey] = containerName

}

func innerReadACSStatisContainerInfo(acsStaticContainerInfoMountPath string) (containers []types.ContainerJSON, removed []string, changed bool, err error) {
	subDirs, err := listSubdirectories(acsStaticContainerInfoMountPath)
	nowIDs := make(map[string]struct{})

	if err == nil {
		if len(subDirs) != 0 {
			for _, subDir := range subDirs {
				fullPtah := acsStaticContainerInfoMountPath + "/" + subDir + "/config.json"
				stat, err := os.Stat(fullPtah)
				if err != nil {
					continue
				}
				body, err := os.ReadFile(filepath.Clean(fullPtah))
				if err != nil {
					continue
				}
				var staticContainerInfo OCIContainerInfo
				err = json.Unmarshal(body, &staticContainerInfo)
				if err != nil {
					continue
				}
				containerName := staticContainerInfo.Annotations[containerNameAnnotationKey]
				if len(containerName) > 0 && containerName == "logtail" {
					continue
				}
				containerType := staticContainerInfo.Annotations[containerTypeAnnotationKey]
				if len(containerType) > 0 && containerType == "sandbox" {
					continue
				}
				nowIDs[subDir] = struct{}{}
				lastContainerInfo, ok := lastContainerName2Info[containerName]
				if ok {
					if lastContainerInfo != string(body) {
						changed = true
						lastContainerName2Info[containerName] = string(body)
					}
				} else {
					changed = true
					lastContainerName2Info[containerName] = string(body)
				}
				containers = append(containers, staticACSContainerInfoToStandard(&staticContainerInfo, stat, subDir))
			}
		}
	}

	for _, lastContainer := range staticContainers {
		if _, ok := nowIDs[lastContainer.ID]; !ok {
			removed = append(removed, lastContainer.ID)
		}
	}
	return containers, removed, changed, err
}

// listSubdirectories 返回指定路径下的所有子目录
func listSubdirectories(rootPath string) ([]string, error) {
	var subdirNames []string
	entries, err := os.ReadDir(rootPath)
	if err != nil {
		return nil, err
	}
	for _, entry := range entries {
		if entry.IsDir() {
			subdirNames = append(subdirNames, entry.Name())
		}
	}
	return subdirNames, nil
}

func tryReadACSStaticContainerInfo() ([]types.ContainerJSON, []string, bool, error) {
	statusChanged := false
	loadStaticContainerOnce.Do(
		func() {
			if lastContainerName2Info == nil {
				lastContainerName2Info = make(map[string]string)
			}
			envPath := os.Getenv(acsStaticContainerInfoMountPathEnvKey)
			if len(envPath) == 0 {
				acsStaticContainerInfoMountPath = defaultAcsStaticContainerInfoMountPath
			} else {
				acsStaticContainerInfoMountPath = envPath
			}
			staticDockerContainers, _, statusChanged, staticDockerContainerError = innerReadACSStatisContainerInfo(acsStaticContainerInfoMountPath)
		},
	)

	for _, container := range staticDockerContainers {
		if container.State.Status == ContainerStatusRunning && !ContainerProcessAlive(container.State.Pid) {
			container.State.Status = ContainerStatusExited
			statusChanged = true
		}
	}

	//subDirs, err := listSubdirectories(acsStaticContainerInfoMountPath)

	newStaticDockerContainer, removedIDs, changed, staticDockerContainerError := innerReadACSStatisContainerInfo(acsStaticContainerInfoMountPath)
	changed = changed || statusChanged

	if staticDockerContainerError == nil {
		staticDockerContainers = newStaticDockerContainer
	}
	//staticDockerContainerLastStat = osStat

	//logger.Info(context.Background(), "static docker container info file changed or last read error, last", staticDockerContainerLastStat,
	//	"now", osStat, "removed ids", removedIDs, "error", staticDockerContainerError)

	return staticDockerContainers, removedIDs, changed, staticDockerContainerError
}
