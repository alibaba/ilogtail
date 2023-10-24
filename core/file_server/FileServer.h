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

    FileDiscoveryConfig GetFileDiscoveryOptions(const std::string& name) const;
    FileReaderConfig GetFileReaderOptions(const std::string& name) const;
    void AddFileDiscoveryOptions(const std::string& name, const FileDiscoveryOptions* opts, const PipelineContext* ctx);
    void AddFileReaderOptions(const std::string& name, const FileReaderOptions* opts, const PipelineContext* ctx);
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
