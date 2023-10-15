#include "config/watcher/ConfigWatcher.h"

#include <unordered_set>
#include <iostream>

#include "logger/Logger.h"
#include "pipeline/PipelineManager.h"

using namespace std;

namespace logtail {
bool ReadFile(const string& filepath, string& content);

ConfigDiff ConfigWatcher::CheckConfigDiff() {
    ConfigDiff diff;
    unordered_set<string> configSet;
    for (const auto& dir : mSourceDir) {
        error_code ec;
        filesystem::file_status s = filesystem::status(dir, ec);
        if (!ec) {
            LOG_WARNING(sLogger,
                        ("failed to get config dir path info", "skip current object")("dir path", dir.string())(
                            "error code", ec.value())("error msg", ec.message()));
            continue;
        }
        if (!filesystem::exists(s)) {
            LOG_WARNING(sLogger, ("config dir path not existed", "skip current object")("dir path", dir.string()));
            continue;
        }
        if (!filesystem::is_directory(s)) {
            LOG_WARNING(sLogger,
                        ("config dir path is not a directory", "skip current object")("dir path", dir.string()));
            continue;
        }
        for (auto const& entry : filesystem::directory_iterator(dir, ec)) {
            const string& filepath = entry.path().string();
            if (!filesystem::is_regular_file(entry.status(ec))) {
                LOG_DEBUG(sLogger, ("config file is not a regular file", "skip current object")("filepath", filepath));
                continue;
            }
            uintmax_t size = filesystem::file_size(entry.path(), ec);
            filesystem::file_time_type mTime = filesystem::last_write_time(entry.path(), ec);

            auto iter = mFileInfoMap.find(filepath);
            if (iter == mFileInfoMap.end()) {
                NewConfig config;
                if (!LoadConfigFromFile(entry.path(), config)) {
                    continue;
                }
                diff.mAdded.push_back(std::move(config));
                mFileInfoMap[filepath] = make_pair(size, mTime);
            } else if (iter->second.first != size || iter->second.second != mTime) {
                NewConfig config;
                if (!LoadConfigFromFile(entry.path(), config)) {
                    continue;
                }
                if (config != PipelineManager::GetInstance()->FindPipelineByName(config.mName)->GetConfig()) {
                    diff.mModified.push_back(std::move(config));
                } else {
                    // 为了插件系统过渡使用
                    diff.mUnchanged.push_back(config.mName);
                }
                mFileInfoMap[filepath] = make_pair(size, mTime);
            } else {
                // 为了插件系统过渡使用
                diff.mUnchanged.push_back(filepath);
            }
            configSet.insert(filepath);
        }
    }
    for (const auto& name : PipelineManager::GetInstance()->GetAllPipelineNames()) {
        if (configSet.find(name) == configSet.end()) {
            diff.mRemoved.push_back(name);
        }
    }
    return diff;
}

void ConfigWatcher::AddSource(const string& dir) {
    mSourceDir.emplace_back(dir);
}

bool ConfigWatcher::LoadConfigFromFile(const filesystem::path& filepath, NewConfig& config) {
    const string& ext = filepath.extension().string();
    if (ext != "yaml" && ext != "yml" && ext != "json") {
        LOG_WARNING(sLogger, ("unsupported config file format", "skip current object")("filepath", filepath));
        return false;
    }
    string content;
    if (!ReadFile(filepath.string(), content)) {
        LOG_WARNING(sLogger, ("failed to open config file", "skip current object")("filepath", filepath));
        return false;
    }
    if (content.empty()) {
        LOG_WARNING(sLogger, ("empty config file", "skip current object")("filepath", filepath));
        return false;
    }
    unique_ptr<Table> table = Table::CreateTable(ext);
    if (!table->Parse(content)) {
        LOG_WARNING(sLogger, ("config file format error", "skip current object")("filepath", filepath));
        return false;
    }
    config.mName = filepath.string();
    config.mDetail = std::move(table);
    return true;
}

bool ReadFile(const string& filepath, string& content) {
    constexpr size_t read_size = size_t(4096);
    ifstream fin(filepath);
    if (!fin) {
        return false;
    }
    string buf = string(read_size, '\0');
    while (fin.read(&buf[0], read_size)) {
        content.append(buf, 0, fin.gcount());
    }
    content.append(buf, 0, fin.gcount());
    return true;
}
} // namespace logtail