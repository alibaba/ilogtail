#ifndef ARGUS_CMS_CLOUD_CHANNEL_H
#define ARGUS_CMS_CLOUD_CHANNEL_H

#include <list>
#include <vector>
#include <chrono>
#include <mutex>

#include "common/InstanceLock.h"
#include "common/ModuleData.h"

#include "core/OutputChannel.h"
#include "core/TaskManager.h"

#include "common/SafeVector.h"

namespace common {
    struct HttpRequest;
    struct HttpResponse;
}
namespace cloudMonitor {
    struct CloudMsg {
        std::string name;
        std::chrono::system_clock::time_point timestamp; // ms
        std::string msg;
    };

    struct CloudMetricConfig {
        std::string acckeyId;
        std::string accKeySecretSign;
        std::string uploadEndpoint;
        std::string secureToken;
        bool needTimestamp = false;
    };

#include "common/test_support"
class CloudChannel : public argus::OutputChannel {
public:
    typedef std::remove_pointer<int (*)(const common::HttpRequest &, common::HttpResponse &)>::type THttpPost;
private:
    explicit CloudChannel(const std::function<THttpPost> &post);
public:
    static const char * const Name;
    CloudChannel();
    ~CloudChannel() override;

    int addMessage(const std::string &name, const std::chrono::system_clock::time_point &timestamp,
                    int exitCode, const std::string &result, const std::string &conf,
                    bool reportStatus, const std::string &mid) override;
    int AddCommonMetrics(const std::vector<common::CommonMetric> &metrics, std::string conf) override;

    void Start();

    uint32_t GetQueueEmptyCount() const {
        return queueEmptyCounter;
    }

private:

    void doRun() override;

    size_t ToPayloadMsgs(const std::vector<CloudMsg> &cloudMsgs, std::string &body) const;
    std::string ToPayloadMetricData(const common::MetricData &metricData,
                                    const std::chrono::system_clock::time_point &timestamp) const;


    static bool ParseResponseResult(const std::string &result);
    static bool ParseCloudMetricConfig(const std::string &conf, CloudMetricConfig &config);

    virtual bool SendMsg(size_t count, const std::string &sendBody);
    virtual int SendMetric(const std::string &body, const CloudMetricConfig &metricConfig) const;

    int GetSendMetricHeader(const std::string &body,
                            const CloudMetricConfig &metricConfig,
                            std::map<std::string, std::string> &header) const;
    size_t GetSendMetricBody(const std::vector<common::CommonMetric> &metrics,
                             std::vector<std::string> &bodyVector,
                             CloudMetricConfig &metricConfig) const;

    bool doSend(const char *, const common::HttpRequest &, common::HttpResponse &, int tryTimes) const;

    argus::NodeItem nodeItem() const;
private:
    mutable std::mutex mutex;
    SafeList<CloudMsg> mMsgs;

    std::vector<argus::MetricItem> mMetricItems;
    argus::NodeItem mNodeItem;
    argus::CloudAgentInfo mCloudAgentInfo;
    std::chrono::seconds mInterval{15};
    size_t mCurrentMetricItemIndex = 0;
    size_t mCurrentMetricItemTryTimes = 0;
    size_t mOkSendCount = 0;
    size_t mErrorSendCount = 0;
    size_t mTotalSendCount = 0;
    size_t mLastContinueErrorCount = 0;
    size_t mMaxMsgSize = 0;
    size_t mMaxMetricSize = 0;
    size_t mMetricSendSize = 0;
    std::string mHostname;
    std::string mMainIp;

    std::atomic<uint32_t> queueEmptyCounter{0};

    const std::function<THttpPost> fnHttpPost;
};
#include "common/test_support"

}
#endif 