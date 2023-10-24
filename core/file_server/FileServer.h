#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "file_server/FileDiscoveryOptions.h"
#include "pipeline/PipelineContext.h"
#include "reader/FileReaderOptions.h"

namespace logtail {

class FileServer {
public:
    FileServer(const FileServer&) = delete;
    FileServer& operator=(const FileServer&) = delete;

    static FileServer* GetInstance() {
        static FileServer instance;
        return &instance;
    }

    FileDiscoveryConfig GetFileDiscoveryConfig(const std::string& name) const;
    FileReaderConfig GetFileReaderConfig(const std::string& name) const;
    void AddFileDiscoveryConfig(const std::string& name, const FileDiscoveryOptions* opts, const PipelineContext* ctx);
    void AddFileReaderConfig(const std::string& name, const FileReaderOptions* opts, const PipelineContext* ctx);
    void ClearFileConfigs() {
        mPipelineNameFileDiscoveryConfigsMap.clear();
        mPipelineNameFileReaderConfigsMap.clear();
    }
    const std::unordered_map<std::string, FileDiscoveryConfig>& GetAllFileDiscoveryConfigs() const {
        return mPipelineNameFileDiscoveryConfigsMap;
    }
    const std::unordered_map<std::string, FileReaderConfig>& GetAllFileReaderConfigs() const {
        return mPipelineNameFileReaderConfigsMap;
    }

private:
    FileServer() = default;
    ~FileServer();

    std::unordered_map<std::string, FileDiscoveryConfig> mPipelineNameFileDiscoveryConfigsMap;
    std::unordered_map<std::string, FileReaderConfig> mPipelineNameFileReaderConfigsMap;
};

} // namespace logtail
