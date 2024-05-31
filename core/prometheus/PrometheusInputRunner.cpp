#include "PrometheusInputRunner.h"

#include "common/TimeUtil.h"
#include "prometheus/Scraper.h"
#include "sdk/Common.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"

using namespace std;

namespace logtail {
PrometheusInputRunner::PrometheusInputRunner() {
}

/// @brief 接收并更新插件注册的每个ScrapeJob
void PrometheusInputRunner::UpdateScrapeInput(const string& inputName, const vector<ScrapeJob>& jobs) {
    set<ScrapeJob> oldScrapeJobsSet = scrapeInputsMap[inputName];
    scrapeInputsMap[inputName].clear();
    for (const ScrapeJob& job : jobs) {
        scrapeInputsMap[inputName].insert(job);
        ScraperGroup::GetInstance()->UpdateScrapeJob(job);
    }
    for (const ScrapeJob& job : oldScrapeJobsSet) {
        if (!scrapeInputsMap[inputName].count(job)) {
            ScraperGroup::GetInstance()->RemoveScrapeJob(job);
        }
    }
}

/// @brief 初始化管理类
void PrometheusInputRunner::Start() {
    ScraperGroup::GetInstance()->Start();
}


/// @brief 停止管理类，资源回收
void PrometheusInputRunner::Stop() {
    ScraperGroup::GetInstance()->Stop();
}

void PrometheusInputRunner::HasRegisteredPlugin() {
}

/// 暂时不做处理
/// 使用读写锁控制
// void PrometheusInputRunner::HoldOn() {
//     mEventLoopThreadRWL.lock();
// }

/// 启动或重启
/// 使用读写锁控制
/// 创建scrapeloop
// void PrometheusInputRunner::Reload() {
//     // StartEventLoop();
//     mEventLoopThreadRWL.unlock();
// }

}; // namespace logtail