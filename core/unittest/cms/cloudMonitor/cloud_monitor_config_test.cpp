#include "cloudMonitor/cloud_monitor_config.h"
#include <string>
#include "common/Config.h"

Config *newCmsConfig(std::string path) {
    auto *config = new Config();
    if (path.empty()) {
        auto baseDir = SingletonConfig::Instance()->getBaseDir();
        path = (baseDir / "local_data" / "conf" / "agent.properties").string();
    }
    config->loadConfig(path);

    return config;
}
