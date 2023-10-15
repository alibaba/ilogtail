#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <filesystem>

#include "config/NewConfig.h"
#include "config/ConfigDiff.h"

namespace logtail {
class ConfigWatcher {
public:
    ConfigWatcher(const ConfigWatcher&) = delete;
    ConfigWatcher& operator=(const ConfigWatcher&) = delete;

    static ConfigWatcher* GetInstance() {
        static ConfigWatcher instance;
        return &instance;
    }

    ConfigDiff CheckConfigDiff();
    void AddSource(const std::string& dir);

private:
    ConfigWatcher() = default;
    ~ConfigWatcher() = default;

    bool LoadConfigFromFile(const std::filesystem::path& filePath, NewConfig& config);

    std::vector<std::filesystem::path> mSourceDir;
    std::map<std::string, std::pair<uintmax_t, std::filesystem::file_time_type>> mFileInfoMap;
};
} // namespace logtail