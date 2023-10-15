#include "file/FileServer.h"

#include "checkpoint/CheckPointManager.h"
#include "common/Flags.h"
#include "config_manager/ConfigManager.h"
#include "controller/EventDispatcher.h"
#include "event_handler/LogInput.h"
#include "polling/PollingDirFile.h"
#include "polling/PollingModify.h"
#include "FileServer.h"

using namespace std;

DEFINE_FLAG_BOOL(enable_polling_discovery, "", true);

namespace logtail {
void FileServer::Start() {
    ConfigManager::GetInstance()->LoadDockerConfig();
    CheckPointManager::Instance()->LoadCheckPoint();
    ConfigManager::GetInstance()->RegisterHandlers();
    EventDispatcher::GetInstance()->AddExistedCheckPointFileEvents();
    if (BOOL_FLAG(enable_polling_discovery)) {
        PollingModify::GetInstance()->Start();
        PollingDirFile::GetInstance()->Start();
    }
    LogInput::GetInstance()->Start();
}

void FileServer::Pause() {
    LogInput::GetInstance()->HoldOn();
    EventDispatcher::GetInstance()->mBrokenLinkSet.clear();
    PollingDirFile::GetInstance()->ClearCache();
    ConfigManager::GetInstance()->ClearFilePipelineMatchCache(); // 原RemoveAllConfigs
    EventDispatcher::GetInstance()->DumpAllHandlersMeta(true);
    CheckPointManager::Instance()->DumpCheckPointToLocal();
    CheckPointManager::Instance()->ResetLastDumpTime();
}

void FileServer::Resume() {
    ClearContainerInfo();
    ConfigManager::GetInstance()->DoUpdateContainerPaths();
    ConfigManager::GetInstance()->SaveDockerConfig();
    LogInput::GetInstance()->Resume(true);
}

void FileServer::AddPipeline(const std::shared_ptr<Pipeline>& pipeline) {
    mPipelineNameEntityMap[pipeline->Name()] = pipeline;
}

void FileServer::RemovePipeline(const std::shared_ptr<Pipeline>& pipeline) {
    mPipelineNameEntityMap.erase(pipeline->Name());
}

void FileServer::SaveContainerInfo(const std::string& pipeline,
                                   std::shared_ptr<std::vector<DockerContainerPath>>& info) {
    mAllDockerContainerPathMap[pipeline] = info;
}

std::shared_ptr<std::vector<DockerContainerPath>> FileServer::GetAndRemoveContainerInfo(const std::string& pipeline) {
    auto iter = mAllDockerContainerPathMap.find(pipeline);
    if (iter == mAllDockerContainerPathMap.end()) {
        return std::make_shared<std::vector<DockerContainerPath>>();
    }
    mAllDockerContainerPathMap.erase(iter);
    return iter->second;
}

void FileServer::ClearContainerInfo() {
    mAllDockerContainerPathMap.clear();
}

FileServer::~FileServer() {
}
} // namespace logtail


