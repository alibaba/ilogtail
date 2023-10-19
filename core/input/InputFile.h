/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "config/DockerFileConfig.h"
#include "plugin/interface/Input.h"

namespace logtail {

class InputFile : public Input {
public:
    enum class Encoding { UTF8, GBK };

    struct Multiline {
        enum class Mode { CUSTOM, JSON };

        Mode mMode = Mode::CUSTOM;
        std::string mStartPattern;
        std::string mContinuePattern;
        std::string mEndPattern;
    };

    struct ContainerFilters {
        std::string mK8sNamespaceRegex;
        std::string mK8sPodRegex;
        std::unordered_map<std::string, std::string> mIncludeK8sLabel;
        std::unordered_map<std::string, std::string> mExcludeK8sLabel;
        std::string mK8sContainerRegex;
        std::unordered_map<std::string, std::string> mIncludeEnv;
        std::unordered_map<std::string, std::string> mExcludeEnv;
        std::unordered_map<std::string, std::string> mIncludeContainerLabel;
        std::unordered_map<std::string, std::string> mExcludeContainerLabel;
    };

    static const std::string sName;

    InputFile();

    const std::string& Name() const override { return sName; }
    // bool Init(const Table& config) override;
    bool Init(const Json::Value& config, Json::Value& optionalGoPipeline) override;
    bool Start() override;
    bool Stop(bool isPipelineRemoving) override;

    std::vector<std::string> mFilePaths;
    int32_t mMaxDirSearchDepth = 0;
    std::vector<std::string> mExcludeFilePaths;
    std::vector<std::string> mExcludeFiles;
    std::vector<std::string> mExcludeDirs;
    bool mTailingAllMatchedFiles = false;
    Encoding mFileEncoding = Encoding::UTF8;
    uint32_t mTailSizeKB;
    Multiline mMultiline;
    bool mEnableContainerDiscovery = false;
    ContainerFilters mContainerFilters;
    std::unordered_map<std::string, std::string> mExternalK8sLabelTag;
    std::unordered_map<std::string, std::string> mExternalEnvTag;
    bool mCollectingContainersMeta = false;
    bool mAppendingLogPositionMeta = false;
    int32_t mPreservedDirDepth = -1;
    uint32_t mFlushTimeoutSecs;
    uint32_t mReadDelaySkipThresholdBytes = 0;
    uint32_t mReadDelayAlertThresholdBytes;
    uint32_t mRotatorQueueSize;
    uint32_t mCloseUnusedReaderIntervalSec;
    bool mAllowingCollectingFilesInRootDir = false;
    bool mAllowingIncludedByMultiConfigs = false;
    uint32_t mMaxCheckpointDirSearchDepth = 0;
    uint32_t mExactlyOnceConcurrency = 0;

private:
    void ParseWildcardPath();
    std::pair<std::string, std::string> GetDirAndFileNameFromPath(const std::string& filePath);
    void GenerateContainerMetaFetchingGoPipeline(Json::Value& res) const;

    std::string mBasePath;
    std::string mFilePattern;
    std::vector<std::string> mConstWildcardPaths;
    std::vector<std::string> mWildcardPaths;
    uint16_t mWildcardDepth;

    // Blacklist control.
    bool mHasBlacklist = false;
    // Absolute path of directories to filter, such as /app/log. It will filter
    // subdirectories as well.
    std::vector<std::string> mDirPathBlacklist;
    // Wildcard (*/?) is included, use fnmatch with FNM_PATHNAME to filter, which
    // will also filter subdirectories. For example, /app/* filters subdirectory
    // /app/log but keep /app/text.log, because /app does not match /app/*. And
    // because /app/log is filtered, so any changes under it will be ignored, so
    // both /app/log/sub and /app/log/text.log will be blacklisted.
    std::vector<std::string> mWildcardDirPathBlacklist;
    // Multiple level wildcard (**) is included, use fnmatch with 0 as flags to filter,
    // which will blacklist /path/a/b with pattern /path/**.
    std::vector<std::string> mMLWildcardDirPathBlacklist;
    // Absolute path of files to filter, */? is supported, such as /app/log/100*.log.
    std::vector<std::string> mFilePathBlacklist;
    // Multiple level wildcard (**) is included.
    std::vector<std::string> mMLFilePathBlacklist;
    // File name only, */? is supported too, such as 100*.log. It is similar to
    // mFilePattern, but works in reversed way.
    std::vector<std::string> mFileNameBlacklist;

    std::shared_ptr<std::vector<DockerContainerPath>> mContainerInfos;
};

} // namespace logtail
