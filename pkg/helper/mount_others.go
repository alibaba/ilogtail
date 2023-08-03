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

//go:build !windows
// +build !windows

package helper

import (
	"os"
	"strings"
)

var DefaultLogtailMountPath string

func GetMountedFilePath(logPath string) string {
	return GetMountedFilePathWithBasePath(DefaultLogtailMountPath, logPath)
}

func GetMountedFilePathWithBasePath(basePath, logPath string) string {
	return basePath + logPath
}

func TryGetRealPath(path string, maxRecurseNum int) string {
	if maxRecurseNum == 0 {
		return ""
	}

	sepLen := len(string(os.PathSeparator))
	index := 0 // assume path is absolute
	for {
		i := strings.IndexRune(path[index+sepLen:], os.PathSeparator)
		if i == -1 {
			// pre condition: os.Stat(path) returns error
			if _, err := os.Lstat(path); err != nil {
				// file does not exist, return directly
				return ""
			}
			// file is a symlink
			target, _ := os.Readlink(path)
			path = GetMountedFilePath(target)
			if _, err := os.Stat(path); err != nil {
				// file referenced does not exist or has symlink, call recursively
				return TryGetRealPath(path, maxRecurseNum-1)
			}
			return path
		} else {
			index += i + sepLen
			if _, err := os.Stat(path[:index]); err != nil {
				if _, err := os.Lstat(path[:index]); err != nil {
					// path[:index] does not exist, return directly
					return ""
				}
				// path[:index] is a symlink
				target, _ := os.Readlink(path[:index])
				partailPath := GetMountedFilePath(target)
				path = partailPath + path[index:]
				if _, err := os.Stat(partailPath); err != nil {
					// path referenced does not exist or has symlink, call recursively
					return TryGetRealPath(path, maxRecurseNum-1)
				}
				if _, err := os.Stat(path); err != nil {
					// perhaps more symlink exists
					index = len(partailPath)
					continue
				}
				return path
			}
		}
	}
}

func init() {
	defaultPath := "/logtail_host"
	_, err := os.Stat(defaultPath)
	// only change default mount path when `/logtail_host` path exists.
	if err == nil {
		DefaultLogtailMountPath = defaultPath
	}
}
