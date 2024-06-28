#include "CmsMain.h"

#include <iostream>
#include <string>
// #include <apr-1/apr_version.h>
// #include <sigar-1.6/sigar.h>
// #include <boost/algorithm/string.hpp>

#include "cms/common/CPoll.h"
#include "cms/common/Common.h"
#include "cms/common/Config.h"
#include "cms/common/FilePathUtils.h"
#include "cms/common/Logger.h"
#include "cms/core/argus_manager.h"

namespace cms {
fs::path getConfigDir() {
    return GetExecDir() / "local_data" / "conf";
}
fs::path getConfigFile() {
    return getConfigDir() / "agent.properties";
}
fs::path getLogDir() {
    return GetExecDir() / "local_data" / "logs";
}

int initLog(const std::string& logFile, const std::function<void(int)>& fnExit = exit) {
    const Config* cfg = SingletonConfig::Instance();

    const fs::path logFilePath = getLogDir() / logFile;
    if (SingletonLogger::Instance()->setup(logFilePath.string())) {
        std::string logLevel = cfg->GetValue("agent.logger.level", "");
        if (logLevel.empty()) {
            logLevel = cfg->GetValue("agent.logger.debug", false) ? "debug" : "info";
        }
        SingletonLogger::Instance()->setLogLevel(logLevel);

        const auto fileSize = cfg->GetValue<size_t>("agent.logger.file.size", 10 * 1000 * 1000);
        SingletonLogger::Instance()->setFileSize(fileSize);

        const int fileCount = cfg->GetValue("agent.logger.file.count", 7);
        SingletonLogger::Instance()->setCount(fileCount);
        return 0;
    } else {
        LogError("init log error, argusagent will exit");
        (fnExit ? fnExit : exit)(1);
        return -1;
    }
}

void StartCmsService() {
    if (common::Poll* pPoll = common::SingletonPoll::Instance(); pPoll != nullptr) {
        pPoll->runIt();
        pPoll->join();
    }
}

void Start(bool async) {
    auto configFile = getConfigFile().string();
    Config* cfg = SingletonConfig::Instance();
    if (cfg->loadConfig(configFile) != 0) {
        std::cout << "load config file(" << configFile << ") failed, will use default!" << std::endl;
    }
    initLog(cfg->GetValue("agent.work.logfile.name", "argusagent.log"));
    LogInfo("--------------------------------------------------------------------------------");
    LogInfo("{} is starting...", common::getVersionDetail());
    LogInfo("BaseDir: {}", cfg->getBaseDir().string());
    cfg->setModuleTaskFile((getConfigDir() / "moduleTask.json").string());

    std::thread thread([]() {
        argus::SingletonArgusManager::Instance()->Start();
        StartCmsService();
    });
    async ? thread.detach() : thread.join();
}
} // namespace cms
