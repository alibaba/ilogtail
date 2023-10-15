#include "config/provider/ConfigProvider.h"

#include "app_config/AppConfig.h"
#include "config/watcher/ConfigWatcher.h"
#include "ConfigProvider.h"

using namespace std;

namespace logtail {
void ConfigProvider::Init(const string& dir) {
    // default path: /etc/ilogtail/config/${dir}
    mSourceDir.assign(AppConfig::GetInstance()->GetLogtailSysConfDir());
    mSourceDir /= "config";
    mSourceDir /= dir;

    error_code ec;
    filesystem::create_directories(mSourceDir, ec);
    ConfigWatcher::GetInstance()->AddSource(mSourceDir);
}
} // namespace logtail


