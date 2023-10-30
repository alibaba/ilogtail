// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "file_server/FileDiscoveryOptions.h"

#include <filesystem>

#include "common/LogtailCommonFlags.h"
#include "common/ParamExtractor.h"

using namespace std;

namespace logtail {

bool FileDiscoveryOptions::Init(const Json::Value& config, const PipelineContext& ctx, const string& pluginName) {
    string errorMsg;
    // FilePaths + MaxDirSearchDepth
    if (!GetMandatoryListParam<string>(config, "FilePaths", mFilePaths, errorMsg)) {
        PARAM_ERROR(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    }
    if (mFilePaths.size() != 1) {
        PARAM_ERROR(ctx.GetLogger(), "param FilePaths has more than 1 element", pluginName, ctx.GetConfigName());
    }
    auto dirAndFile = GetDirAndFileNameFromPath(mFilePaths[0]);
    mBasePath = dirAndFile.first, mFilePattern = dirAndFile.second;
    if (mBasePath.empty() || mFilePattern.empty()) {
        PARAM_ERROR(ctx.GetLogger(), "param FilePaths[0] is invalid", pluginName, ctx.GetConfigName());
    }
    size_t len = mBasePath.size();
    if (len > 2 && mBasePath[len - 1] == '*' && mBasePath[len - 2] == '*'
        && mBasePath[len - 3] == filesystem::path::preferred_separator) {
        if (len == 3) {
            // for parent path like /**, we should maintain root dir, i.e., /
            mBasePath = mBasePath.substr(0, len - 2);
        } else {
            mBasePath = mBasePath.substr(0, len - 3);
        }
        // MaxDirSearchDepth is only valid when parent path ends with **
        if (!GetOptionalIntParam(config, "MaxDirSearchDepth", mMaxDirSearchDepth, errorMsg)) {
            PARAM_WARNING_DEFAULT(ctx.GetLogger(), errorMsg, 0, pluginName, ctx.GetConfigName());
        }
    }
    ParseWildcardPath();

    // PreservedDirDepth
    if (!GetOptionalIntParam(config, "PreservedDirDepth", mPreservedDirDepth, errorMsg)) {
        PARAM_WARNING_DEFAULT(ctx.GetLogger(), errorMsg, -1, pluginName, ctx.GetConfigName());
    }

    // ExcludeFilePaths
    if (!GetOptionalListParam<string>(config, "ExcludeFilePaths", mExcludeFilePaths, errorMsg)) {
        PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    } else {
        for (size_t i = 0; i < mExcludeFilePaths.size(); ++i) {
            if (!filesystem::path(mExcludeFilePaths[i]).is_absolute()) {
                PARAM_WARNING_IGNORE(ctx.GetLogger(),
                                     "ExcludeFilePaths[" + ToString(i) + "] is not absolute",
                                     pluginName,
                                     ctx.GetConfigName());
                continue;
            }
            bool isMultipleLevelWildcard = mExcludeFilePaths[i].find("**") != std::string::npos;
            if (isMultipleLevelWildcard) {
                mMLFilePathBlacklist.push_back(mExcludeFilePaths[i]);
            } else {
                mFilePathBlacklist.push_back(mExcludeFilePaths[i]);
            }
        }
    }

    // ExcludeFiles
    if (!GetOptionalListParam<string>(config, "ExcludeFiles", mExcludeFiles, errorMsg)) {
        PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    } else {
        for (size_t i = 0; i < mExcludeFiles.size(); ++i) {
            if (mExcludeFiles[i].find(filesystem::path::preferred_separator) != std::string::npos) {
                PARAM_WARNING_IGNORE(ctx.GetLogger(),
                                     "ExcludeFiles[" + ToString(i) + "] contains path separator",
                                     pluginName,
                                     ctx.GetConfigName());
                continue;
            }
            mFileNameBlacklist.push_back(mExcludeFiles[i]);
        }
    }

    // ExcludeDirs
    if (!GetOptionalListParam<string>(config, "ExcludeDirs", mExcludeDirs, errorMsg)) {
        PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    } else {
        for (size_t i = 0; i < mExcludeDirs.size(); ++i) {
            if (!filesystem::path(mExcludeDirs[i]).is_absolute()) {
                PARAM_WARNING_IGNORE(ctx.GetLogger(),
                                     "ExcludeDirs[" + ToString(i) + "] is not absolute",
                                     pluginName,
                                     ctx.GetConfigName());
                continue;
            }
            bool isMultipleLevelWildcard = mExcludeDirs[i].find("**") != std::string::npos;
            if (isMultipleLevelWildcard) {
                mMLWildcardDirPathBlacklist.push_back(mExcludeDirs[i]);
                continue;
            }
            bool isWildcardPath
                = mExcludeDirs[i].find("*") != std::string::npos || mExcludeDirs[i].find("?") != std::string::npos;
            if (isWildcardPath) {
                mWildcardDirPathBlacklist.push_back(mExcludeDirs[i]);
            } else {
                mDirPathBlacklist.push_back(mExcludeDirs[i]);
            }
        }
    }
    if (!mDirPathBlacklist.empty() || !mWildcardDirPathBlacklist.empty() || !mMLWildcardDirPathBlacklist.empty()
        || !mMLFilePathBlacklist.empty() || !mFileNameBlacklist.empty() || !mFilePathBlacklist.empty()) {
        mHasBlacklist = true;
    }

    // AllowingCollectingFilesInRootDir
    if (!GetOptionalBoolParam(
            config, "AllowingCollectingFilesInRootDir", mAllowingCollectingFilesInRootDir, errorMsg)) {
        PARAM_WARNING_DEFAULT(ctx.GetLogger(), errorMsg, false, pluginName, ctx.GetConfigName());
    } else if (mAllowingCollectingFilesInRootDir) {
        BOOL_FLAG(enable_root_path_collection) = mAllowingCollectingFilesInRootDir;
    }

    // AllowingIncludedByMultiConfigs
    if (!GetOptionalBoolParam(config, "AllowingIncludedByMultiConfigs", mAllowingIncludedByMultiConfigs, errorMsg)) {
        PARAM_WARNING_DEFAULT(ctx.GetLogger(), errorMsg, false, pluginName, ctx.GetConfigName());
    }

    return true;
}

pair<string, string> FileDiscoveryOptions::GetDirAndFileNameFromPath(const string& filePath) {
    filesystem::path path(filePath);
    if (path.is_relative()) {
        error_code ec;
        path = filesystem::absolute(path, ec);
    }
    path = path.lexically_normal();
    return make_pair(path.parent_path().string(), path.filename().string());
}

void FileDiscoveryOptions::ParseWildcardPath() {
    mWildcardPaths.clear();
    mConstWildcardPaths.clear();
    mWildcardDepth = 0;
    if (mBasePath.size() == 0)
        return;
    size_t posA = mBasePath.find('*', 0);
    size_t posB = mBasePath.find('?', 0);
    size_t pos;
    if (posA == std::string::npos) {
        if (posB == std::string::npos)
            return;
        else
            pos = posB;
    } else {
        if (posB == std::string::npos)
            pos = posA;
        else
            pos = std::min(posA, posB);
    }
    if (pos == 0)
        return;
    pos = mBasePath.rfind(filesystem::path::preferred_separator, pos);
    if (pos == std::string::npos)
        return;

        // Check if there is only one path separator, for Windows, the first path
        // separator is next to the first ':'.
#if defined(__linux__)
    if (pos == 0)
#elif defined(_MSC_VER)
    if (pos - 1 == mBasePath.find(':'))
#endif
    {
        mWildcardPaths.push_back(mBasePath.substr(0, pos + 1));
    } else
        mWildcardPaths.push_back(mBasePath.substr(0, pos));
    while (true) {
        size_t nextPos = mBasePath.find(filesystem::path::preferred_separator, pos + 1);
        if (nextPos == std::string::npos)
            break;
        mWildcardPaths.push_back(mBasePath.substr(0, nextPos));
        std::string dirName = mBasePath.substr(pos + 1, nextPos - pos - 1);
        if (dirName.find('?') == std::string::npos && dirName.find('*') == std::string::npos) {
            mConstWildcardPaths.push_back(dirName);
        } else {
            mConstWildcardPaths.push_back("");
        }
        pos = nextPos;
    }
    mWildcardPaths.push_back(mBasePath);
    if (pos < mBasePath.size()) {
        std::string dirName = mBasePath.substr(pos + 1);
        if (dirName.find('?') == std::string::npos && dirName.find('*') == std::string::npos) {
            mConstWildcardPaths.push_back(dirName);
        } else {
            mConstWildcardPaths.push_back("");
        }
    }

    for (size_t i = 0; i < mBasePath.size(); ++i) {
        if (filesystem::path::preferred_separator == mBasePath[i])
            ++mWildcardDepth;
    }
}

} // namespace logtail
