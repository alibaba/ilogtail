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

//go:build linux
// +build linux

package helper

import (
	"context"
	"fmt"
	"strconv"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/util"
)

func ContainerProcessAlive(pid int) bool {
	procStatPath := GetMountedFilePath(fmt.Sprintf("/proc/%d/stat", pid))
	logger.Info(context.Background(), "ContainerProcessAlive", procStatPath)
	exist, err := util.PathExists(procStatPath)
	logger.Info(context.Background(), strconv.FormatBool(exist), err)
	if err != nil {
		logger.Error(context.Background(), "DETECT_CONTAINER_ALARM", "stat container proc path", procStatPath, "error", err)
	} else if !exist {
		return false
	}
	return true
}
