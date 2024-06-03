#include "ScrapeWork.h"

#include "common/TimeUtil.h"
#include "logger/Logger.h"
#include "prom/TextParser.h"
#include "queue/ProcessQueueItem.h"
#include "queue/ProcessQueueManager.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"

using namespace std;

namespace logtail {

ScrapeWork::ScrapeWork() {
    client = std::make_unique<sdk::CurlClient>();
    finished = false;
}

ScrapeWork::ScrapeWork(const ScrapeTarget& target) : target(target), finished(false) {
    client = std::make_unique<sdk::CurlClient>();
}

void ScrapeWork::StartScrapeLoop() {
    // 以线程的方式实现
    if (!mScrapeLoopThread) {
        mScrapeLoopThread = CreateThread([this]() { scrapeLoop(); });
    }
}

void ScrapeWork::StopScrapeLoop() {
    finished = true;
}

/// @brief scrape target loop
void ScrapeWork::scrapeLoop() {
    printf("start prometheus scrape loop\n");
    uint64_t randSleep = getRandSleep();
    printf("randSleep %llums\n", randSleep / 1000ULL / 1000ULL);
    std::this_thread::sleep_for(std::chrono::nanoseconds(randSleep));

    while (!finished) {
        uint64_t lastProfilingTime = GetCurrentTimeInNanoSeconds();
        // ReadLock lock(mScrapeLoopThreadRWL);
        printf("scrape time %llus\n", lastProfilingTime / 1000ULL / 1000ULL / 1000ULL);

        // scrape target by CurlClient
        auto httpResponse = scrape();

        // text parser
        const auto& sourceBuffer = make_shared<SourceBuffer>();
        auto parser = TextParser(sourceBuffer);
        auto eventGroup = parser.Parse(httpResponse.content);

        // 发送到对应的处理队列
        pushEventGroup(std::move(*eventGroup));

        uint64_t elapsedTime = GetCurrentTimeInNanoSeconds() - lastProfilingTime;
        uint64_t timeToWait = target.scrapeInterval * 1000ULL * 1000ULL * 1000ULL - elapsedTime;
        printf("time to wait: %llums\n", timeToWait / 1000ULL / 1000ULL);
        std::this_thread::sleep_for(std::chrono::nanoseconds(timeToWait));
    }
    printf("stop loop\n");
}

inline uint64_t ScrapeWork::getRandSleep() {
    // 根据target信息构造hash输入
    string key = string(target.jobName + target.scheme + target.host + target.metricsPath);
    // 遇到性能问题可以考虑使用xxhash代替Boost.hash优化
    boost::hash<std::string> string_hash;
    uint64_t h = string_hash(key);
    uint64_t randSleep = ((double)1.0) * target.scrapeInterval * (1.0 * h / (uint64_t)0xFFFFFFFFFFFFFFFF);
    uint64_t sleepOffset = GetCurrentTimeInNanoSeconds() % (target.scrapeInterval * 1000ULL * 1000ULL * 1000ULL);
    if (randSleep < sleepOffset) {
        randSleep += target.scrapeInterval * 1000ULL * 1000ULL * 1000ULL;
    }
    randSleep -= sleepOffset;
    return randSleep;
}

inline sdk::HttpMessage ScrapeWork::scrape() {
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
    return httpResponse;
}

void ScrapeWork::pushEventGroup(PipelineEventGroup eGroup) {
    // parser.Parse返回unique_ptr但下面的构造函数接收右值引用，所以又一次拷贝消耗
    auto item = std::make_unique<ProcessQueueItem>(std::move(eGroup), target.inputIndex);
    for (size_t i = 0; i < 1000; ++i) {
        if (ProcessQueueManager::GetInstance()->PushQueue(target.queueKey, std::move(item)) == 0) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

} // namespace logtail
