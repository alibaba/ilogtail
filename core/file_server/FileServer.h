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

    std::pair<const FileDiscoveryOptions*, const PipelineContext*>
    GetFileDiscoveryOptions(const std::string& name) const;
    std::pair<const FileReaderOptions*, const PipelineContext*> GetFileReaderOptions(const std::string& name) const;
    void FileServer::AddFileDiscoveryOptions(const std::string& name,
                                             const FileDiscoveryOptions* opts,
                                             const PipelineContext* ctx);
    void FileServer::AddFileReaderOptions(const std::string& name,
                                          const FileReaderOptions* opts,
                                          const PipelineContext* ctx);
    void ClearOptions() {
        mPipelineNameFileDiscoveryOptionsMap.clear();
        mPipelineNameFileReaderOptionsMap.clear();
    }
    const std::unordered_map<std::string, std::pair<const FileDiscoveryOptions*, const PipelineContext*>>&
    GetAllFileDiscoveryOptions() const {
        return mPipelineNameFileDiscoveryOptionsMap;
    }

private:
    FileServer() = default;
    ~FileServer();
    
    std::unordered_map<std::string, std::pair<const FileDiscoveryOptions*, const PipelineContext*>>
        mPipelineNameFileDiscoveryOptionsMap;
    std::unordered_map<std::string, std::pair<const FileReaderOptions*, const PipelineContext*>>
        mPipelineNameFileReaderOptionsMap;
};

} // namespace logtail
