#include "ScrapeWork.h"

#include "common/TimeUtil.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"

using namespace std;

logtail::ScrapeWork::ScrapeWork() {
    client = std::make_unique<sdk::CurlClient>();
}

logtail::ScrapeWork::ScrapeWork(const ScrapeTarget& target) : target(target), finished(false) {
    client = std::make_unique<sdk::CurlClient>();
}

void logtail::ScrapeWork::StartScrapeLoop() {
    // 以线程的方式实现
    if (!mScrapeLoopThread) {
        mScrapeLoopThread = CreateThread([this]() { scrapeLoop(); });
    }
}

void logtail::ScrapeWork::StopScrapeLoop() {
    finished = true;
}

/// @brief scrape target loop
void logtail::ScrapeWork::scrapeLoop() {
    printf("start prometheus scrape loop\n");
    while (!finished) {
        uint64_t lastProfilingTime = GetCurrentTimeInNanoSeconds();
        ReadLock lock(mScrapeLoopThreadRWL);
        std::map<std::string, std::string> httpHeader;
        string reqBody;
        sdk::HttpMessage httpResponse;
        httpResponse.header[sdk::X_LOG_REQUEST_ID] = "PrometheusScrapeWork";
        // 使用CurlClient抓取目标
        try {
            client->Send(sdk::HTTP_GET,
                         target.host,
                         target.port,
                         target.metricsPath,
                         "",
                         httpHeader,
                         reqBody,
                         target.scrapeTimeout,
                         httpResponse,
                         "",
                         false);
        } catch (const sdk::LOGException& e) {
            printf("errCode %s, errMsg %s \n", e.GetErrorCode().c_str(), e.GetMessage().c_str());
        }

        uint64_t elapsedTime = GetCurrentTimeInNanoSeconds() - lastProfilingTime;
        uint64_t timeToWait = target.scrapeInterval * 1000ULL * 1000ULL * 1000ULL - elapsedTime;
        std::this_thread::sleep_for(std::chrono::nanoseconds(timeToWait));
    }
    printf("stop loop\n");
}
