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

#if defined(__linux__)
#include <fnmatch.h>
#endif

#include "common/LogtailCommonFlags.h"
#include "common/ParamExtractor.h"
#include "common/StringTools.h"
#include "common/FileSystemUtil.h"

using namespace std;

namespace logtail {

// basePath must not stop with '/'
inline bool _IsSubPath(const std::string& basePath, const std::string& subPath) {
    size_t pathSize = subPath.size();
    size_t basePathSize = basePath.size();
    if (pathSize >= basePathSize && memcmp(subPath.c_str(), basePath.c_str(), basePathSize) == 0) {
        return pathSize == basePathSize || PATH_SEPARATOR[0] == subPath[basePathSize];
    }
    return false;
}

inline bool _IsPathMatched(const std::string& basePath, const std::string& path, int maxDepth) {
    size_t pathSize = path.size();
    size_t basePathSize = basePath.size();
    if (pathSize >= basePathSize && memcmp(path.c_str(), basePath.c_str(), basePathSize) == 0) {
        // need check max_depth + preserve depth
        if (pathSize == basePathSize) {
            return true;
        }
        // like /log  --> /log/a/b , maxDepth 2, true
        // like /log  --> /log1/a/b , maxDepth 2, false
        // like /log  --> /log/a/b/c , maxDepth 2, false
        else if (PATH_SEPARATOR[0] == path[basePathSize]) {
            if (maxDepth < 0) {
                return true;
            }
            int depth = 0;
            for (size_t i = basePathSize; i < pathSize; ++i) {
                if (PATH_SEPARATOR[0] == path[i]) {
                    ++depth;
                    if (depth > maxDepth) {
                        return false;
                    }
                }
            }
            return true;
        }
    }
    return false;
}

static bool isNotSubPath(const std::string& basePath, const std::string& path) {
    size_t pathSize = path.size();
    size_t basePathSize = basePath.size();
    if (pathSize < basePathSize || memcmp(path.c_str(), basePath.c_str(), basePathSize) != 0) {
        return true;
    }

    // For wildcard Windows path C:\*, mWildcardPaths[0] will be C:\, will
    //   fail on following check because of path[basePathSize].
    // Advaned check for such case if flag enable_root_path_collection is enabled.
    auto checkPos = basePathSize;
#if defined(_MSC_VER)
    if (BOOL_FLAG(enable_root_path_collection)) {
        if (basePathSize >= 2 && basePath[basePathSize - 1] == PATH_SEPARATOR[0] && basePath[basePathSize - 2] == ':') {
            --checkPos;
        }
    }
#endif
    return basePathSize > 1 && pathSize > basePathSize && path[checkPos] != PATH_SEPARATOR[0];
}

bool FileDiscoveryOptions::CompareByPathLength(pair<const FileDiscoveryOptions*, const PipelineContext*> left,
                                               pair<const FileDiscoveryOptions*, const PipelineContext*> right) {
    int32_t leftDepth = 0;
    int32_t rightDepth = 0;
    for (size_t i = 0; i < (left.first->mBasePath).size(); ++i) {
        if (PATH_SEPARATOR[0] == (left.first->mBasePath)[i]) {
            leftDepth++;
        }
    }
    for (size_t i = 0; i < (right.first->mBasePath).size(); ++i) {
        if (PATH_SEPARATOR[0] == (right.first->mBasePath)[i]) {
            rightDepth++;
        }
    }
    return leftDepth > rightDepth;
}

bool FileDiscoveryOptions::CompareByDepthAndCreateTime(
    pair<const FileDiscoveryOptions*, const PipelineContext*> left,
    pair<const FileDiscoveryOptions*, const PipelineContext*> right) {
    int32_t leftDepth = 0;
    int32_t rightDepth = 0;
    for (size_t i = 0; i < (left.first->mBasePath).size(); ++i) {
        if (PATH_SEPARATOR[0] == (left.first->mBasePath)[i]) {
            leftDepth++;
        }
    }
    for (size_t i = 0; i < (right.first->mBasePath).size(); ++i) {
        if (PATH_SEPARATOR[0] == (right.first->mBasePath)[i]) {
            rightDepth++;
        }
    }
    if (leftDepth > rightDepth) {
        return true;
    }
    if (leftDepth == rightDepth) {
        return left.second->GetCreateTime() < right.second->GetCreateTime();
    }
    return false;
}

bool FileDiscoveryOptions::Init(const Json::Value& config, const PipelineContext& ctx, const string& pluginName) {
    string errorMsg;
    // FilePaths + MaxDirSearchDepth
    if (!GetMandatoryListParam<string>(config, "FilePaths", mFilePaths, errorMsg)) {
        PARAM_ERROR_RETURN(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    }
    if (mFilePaths.size() != 1) {
        PARAM_ERROR_RETURN(ctx.GetLogger(), "param FilePaths has more than 1 element", pluginName, ctx.GetConfigName());
    }
    auto dirAndFile = GetDirAndFileNameFromPath(mFilePaths[0]);
    mBasePath = dirAndFile.first, mFilePattern = dirAndFile.second;
    if (mBasePath.empty() || mFilePattern.empty()) {
        PARAM_ERROR_RETURN(ctx.GetLogger(), "param FilePaths[0] is invalid", pluginName, ctx.GetConfigName());
    }
#if defined(_MSC_VER)
    mBasePath = EncodingConverter::GetInstance()->FromUTF8ToACP(mBasePath);
    mFilePattern = EncodingConverter::GetInstance()->FromUTF8ToACP(mFilePattern);
#endif
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

bool FileDiscoveryOptions::IsDirectoryInBlacklist(const std::string& dirPath) const {
    if (!mHasBlacklist) {
        return false;
    }

    for (auto& dp : mDirPathBlacklist) {
        if (_IsSubPath(dp, dirPath)) {
            return true;
        }
    }
    for (auto& dp : mWildcardDirPathBlacklist) {
        if (0 == fnmatch(dp.c_str(), dirPath.c_str(), FNM_PATHNAME)) {
            return true;
        }
    }
    for (auto& dp : mMLWildcardDirPathBlacklist) {
        if (0 == fnmatch(dp.c_str(), dirPath.c_str(), 0)) {
            return true;
        }
    }
    return false;
}

bool FileDiscoveryOptions::IsObjectInBlacklist(const std::string& path, const std::string& name) const {
    if (!mHasBlacklist) {
        return false;
    }

    if (IsDirectoryInBlacklist(path)) {
        return true;
    }
    if (name.empty()) {
        return false;
    }

    auto const filePath = PathJoin(path, name);
    for (auto& fp : mFilePathBlacklist) {
        if (0 == fnmatch(fp.c_str(), filePath.c_str(), FNM_PATHNAME)) {
            return true;
        }
    }
    for (auto& fp : mMLFilePathBlacklist) {
        if (0 == fnmatch(fp.c_str(), filePath.c_str(), 0)) {
            return true;
        }
    }

    return false;
}

bool FileDiscoveryOptions::IsFileNameInBlacklist(const std::string& fileName) const {
    if (!mHasBlacklist) {
        return false;
    }

    for (auto& pattern : mFileNameBlacklist) {
        if (0 == fnmatch(pattern.c_str(), fileName.c_str(), 0)) {
            return true;
        }
    }
    return false;
}

// IsMatch checks if the object is matched with current config.
// @path: absolute path of location in where the object stores in.
// @name: the name of the object. If the object is directory, this parameter will be empty.
bool FileDiscoveryOptions::IsMatch(const std::string& path, const std::string& name) const {
    // Check if the file name is matched or blacklisted.
    if (!name.empty()) {
        if (fnmatch(mFilePattern.c_str(), name.c_str(), 0) != 0)
            return false;
        if (IsFileNameInBlacklist(name)) {
            return false;
        }
    }

    // File in docker.
    if (mEnableContainerDiscovery) {
        if (mWildcardPaths.size() > (size_t)0) {
            DockerContainerPath* containerPath = GetContainerPathByLogPath(path);
            if (containerPath == NULL) {
                return false;
            }
            // convert Logtail's real path to config path. eg /host_all/var/lib/xxx/home/admin/logs -> /home/admin/logs
            if (mWildcardPaths[0].size() == (size_t)1) {
                // if mWildcardPaths[0] is root path, do not add mWildcardPaths[0]
                return IsWildcardPathMatch(path.substr(containerPath->mContainerPath.size()), name);
            } else {
                std::string convertPath = mWildcardPaths[0] + path.substr(containerPath->mContainerPath.size());
                return IsWildcardPathMatch(convertPath, name);
            }
        }

        // Normal base path.
        for (size_t i = 0; i < mContainerInfos->size(); ++i) {
            const std::string& containerBasePath = (*mContainerInfos)[i].mContainerPath;
            if (_IsPathMatched(containerBasePath, path, mMaxDirSearchDepth)) {
                if (!mHasBlacklist) {
                    return true;
                }

                // ContainerBasePath contains base path, remove it.
                auto pathInContainer = mBasePath + path.substr(containerBasePath.size());
                if (!IsObjectInBlacklist(pathInContainer, name))
                    return true;
            }
        }
        return false;
    }

    // File not in docker: wildcard or non-wildcard.
    if (mWildcardPaths.empty()) {
        return _IsPathMatched(mBasePath, path, mMaxDirSearchDepth) && !IsObjectInBlacklist(path, name);
    } else
        return IsWildcardPathMatch(path, name);
}

bool FileDiscoveryOptions::IsWildcardPathMatch(const std::string& path, const std::string& name) const {
    size_t pos = 0;
    int16_t d = 0;
    int16_t maxWildcardDepth = mWildcardDepth + 1;
    while (d < maxWildcardDepth) {
        pos = path.find(PATH_SEPARATOR[0], pos);
        if (pos == std::string::npos)
            break;
        ++d;
        ++pos;
    }

    if (d < mWildcardDepth)
        return false;
    else if (d == mWildcardDepth) {
        return fnmatch(mBasePath.c_str(), path.c_str(), FNM_PATHNAME) == 0 && !IsObjectInBlacklist(path, name);
    } else if (pos > 0) {
        if (!(fnmatch(mBasePath.c_str(), path.substr(0, pos - 1).c_str(), FNM_PATHNAME) == 0
              && !IsObjectInBlacklist(path, name))) {
            return false;
        }
    } else
        return false;

    // Only pos > 0 will reach here, which means the level of path is deeper than base,
    // need to check max depth.
    if (mMaxDirSearchDepth < 0)
        return true;
    int depth = 1;
    while (depth < mMaxDirSearchDepth + 1) {
        pos = path.find(PATH_SEPARATOR[0], pos);
        if (pos == std::string::npos)
            return true;
        ++depth;
        ++pos;
    }
    return false;
}

// XXX: assume path is a subdir under mBasePath
bool FileDiscoveryOptions::IsTimeout(const std::string& path) const {
    if (mPreservedDirDepth < 0 || mWildcardPaths.size() > 0)
        return false;

    // we do not check if (path.find(mBasePath) == 0)
    size_t pos = mBasePath.size();
    int depthCount = 0;
    while ((pos = path.find(PATH_SEPARATOR[0], pos)) != std::string::npos) {
        ++depthCount;
        ++pos;
        if (depthCount > mPreservedDirDepth)
            return true;
    }
    return false;
}

bool FileDiscoveryOptions::WithinMaxDepth(const std::string& path) const {
    // default -1 to compatible with old version
    if (mMaxDirSearchDepth < 0)
        return true;
    if (mEnableContainerDiscovery) {
        // docker file, should check is match
        return IsMatch(path, "");
    }

    {
        const auto& base = mWildcardPaths.empty() ? mBasePath : mWildcardPaths[0];
        if (isNotSubPath(base, path)) {
            LOG_ERROR(sLogger, ("path is not child of basePath, path", path)("basePath", base));
            return false;
        }
    }

    if (mWildcardPaths.size() == 0) {
        size_t pos = mBasePath.size();
        int depthCount = 0;
        while ((pos = path.find(PATH_SEPARATOR, pos)) != std::string::npos) {
            ++depthCount;
            ++pos;
            if (depthCount > mMaxDirSearchDepth)
                return false;
        }
    } else {
        int32_t depth = 0 - mWildcardDepth;
        for (size_t i = 0; i < path.size(); ++i) {
            if (path[i] == PATH_SEPARATOR[0])
                ++depth;
        }
        if (depth < 0) {
            LOG_ERROR(sLogger, ("invalid sub dir", path)("basePath", mBasePath));
            return false;
        } else if (depth > mMaxDirSearchDepth)
            return false;
        else {
            // Windows doesn't support double *, so we have to check this.
            auto basePath = mBasePath;
            if (basePath.empty() || basePath.back() != '*')
                basePath += '*';
            if (fnmatch(basePath.c_str(), path.c_str(), 0) != 0) {
                LOG_ERROR(sLogger, ("invalid sub dir", path)("basePath", mBasePath));
                return false;
            }
        }
    }
    return true;
}

DockerContainerPath* FileDiscoveryOptions::GetContainerPathByLogPath(const std::string& logPath) const {
    if (!mContainerInfos) {
        return NULL;
    }
    for (size_t i = 0; i < mContainerInfos->size(); ++i) {
        if (_IsSubPath((*mContainerInfos)[i].mContainerPath, logPath)) {
            return &(*mContainerInfos)[i];
        }
    }
    return NULL;
}

bool FileDiscoveryOptions::IsSameDockerContainerPath(const std::string& paramsJSONStr, bool allFlag) const {
    if (!mEnableContainerDiscovery)
        return true;

    if (!allFlag) {
        DockerContainerPath dockerContainerPath;
        if (!DockerContainerPath::ParseByJSONStr(paramsJSONStr, dockerContainerPath)) {
            LOG_ERROR(sLogger, ("invalid docker container params", "skip this path")("params", paramsJSONStr));
            return true;
        }
        // try update
        for (size_t i = 0; i < mContainerInfos->size(); ++i) {
            if ((*mContainerInfos)[i] == dockerContainerPath) {
                return true;
            }
        }
        return false;
    }

    // check all
    std::unordered_map<std::string, DockerContainerPath> allPathMap;
    if (!DockerContainerPath::ParseAllByJSONStr(paramsJSONStr, allPathMap)) {
        LOG_ERROR(sLogger, ("invalid all docker container params", "skip this path")("params", paramsJSONStr));
        return true;
    }

    // need add
    if (mContainerInfos->size() != allPathMap.size()) {
        return false;
    }

    for (size_t i = 0; i < mContainerInfos->size(); ++i) {
        std::unordered_map<std::string, DockerContainerPath>::iterator iter
            = allPathMap.find((*mContainerInfos)[i].mContainerID);
        // need delete
        if (iter == allPathMap.end()) {
            return false;
        }
        // need update
        if ((*mContainerInfos)[i] != iter->second) {
            return false;
        }
    }
    // same
    return true;
}

bool FileDiscoveryOptions::UpdateDockerContainerPath(const std::string& paramsJSONStr, bool allFlag) {
    if (!mContainerInfos)
        return false;

    if (!allFlag) {
        DockerContainerPath dockerContainerPath;
        if (!DockerContainerPath::ParseByJSONStr(paramsJSONStr, dockerContainerPath)) {
            LOG_ERROR(sLogger, ("invalid docker container params", "skip this path")("params", paramsJSONStr));
            return false;
        }
        // try update
        for (size_t i = 0; i < mContainerInfos->size(); ++i) {
            if ((*mContainerInfos)[i].mContainerID == dockerContainerPath.mContainerID) {
                // update
                (*mContainerInfos)[i] = dockerContainerPath;
                return true;
            }
        }
        // add
        mContainerInfos->push_back(dockerContainerPath);
        return true;
    }

    std::unordered_map<std::string, DockerContainerPath> allPathMap;
    if (!DockerContainerPath::ParseAllByJSONStr(paramsJSONStr, allPathMap)) {
        LOG_ERROR(sLogger, ("invalid all docker container params", "skip this path")("params", paramsJSONStr));
        return false;
    }
    // if update all, clear and reset
    mContainerInfos->clear();
    for (std::unordered_map<std::string, DockerContainerPath>::iterator iter = allPathMap.begin();
         iter != allPathMap.end();
         ++iter) {
        mContainerInfos->push_back(iter->second);
    }
    return true;
}

bool FileDiscoveryOptions::DeleteDockerContainerPath(const std::string& paramsJSONStr) {
    if (!mContainerInfos)
        return false;

    DockerContainerPath dockerContainerPath;
    if (!DockerContainerPath::ParseByJSONStr(paramsJSONStr, dockerContainerPath)) {
        return false;
    }
    for (std::vector<DockerContainerPath>::iterator iter = mContainerInfos->begin(); iter != mContainerInfos->end();
         ++iter) {
        if (iter->mContainerID == dockerContainerPath.mContainerID) {
            mContainerInfos->erase(iter);
            break;
        }
    }
    return true;
}


} // namespace logtail
