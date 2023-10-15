#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "pipeline/Pipeline.h"
#include "config/DockerFileConfig.h"

namespace logtail {
class FileServer {
public:
    FileServer(const FileServer&) = delete;
    FileServer& operator=(const FileServer&) = delete;

    static FileServer* GetInstance() {
        static FileServer instance;
        return &instance;
    }

    void Start();
    void Pause();
    void Resume(); // 临时使用
    void AddPipeline(const std::shared_ptr<Pipeline>& pipeline);
    void RemovePipeline(const std::shared_ptr<Pipeline>& pipeline);

    void SaveContainerInfo(const std::string& pipeline, std::shared_ptr<std::vector<DockerContainerPath>>& info);
    std::shared_ptr<std::vector<DockerContainerPath>> GetAndRemoveContainerInfo(const std::string& pipeline);
    void ClearContainerInfo();
private:
    FileServer() = default;
    ~FileServer();

    std::unordered_map<std::string, std::shared_ptr<Pipeline>> mPipelineNameEntityMap;
    std::unordered_map<std::string, std::shared_ptr<std::vector<DockerContainerPath>>> mAllDockerContainerPathMap;
};

} // namespace logtail