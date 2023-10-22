#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "json/json.h"

#include "pipeline/PipelineContext.h"

namespace logtail {

class FileDiscoveryOptions {
public:
    bool Init(const Json::Value& config, const PipelineContext& ctx, const std::string& pluginName);
    const std::string& GetBasePath() const { return mBasePath; }
    const std::string& GetFilePattern() const { return mFilePattern; }
    const std::vector<std::string>& GetWilecardPaths() const { return mConstWildcardPaths; }

    std::vector<std::string> mFilePaths;
    int32_t mMaxDirSearchDepth = 0;
    int32_t mPreservedDirDepth = -1;
    std::vector<std::string> mExcludeFilePaths;
    std::vector<std::string> mExcludeFiles;
    std::vector<std::string> mExcludeDirs;
    bool mAllowingCollectingFilesInRootDir = false;
    bool mAllowingIncludedByMultiConfigs = false;

private:
    void ParseWildcardPath();
    std::pair<std::string, std::string> GetDirAndFileNameFromPath(const std::string& filePath);

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
};

class FileDiscoveryConfig {
public:
private:
};

} // namespace logtail
