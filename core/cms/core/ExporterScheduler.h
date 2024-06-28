//
// Created by 韩呈杰 on 2024/2/23.
//

#ifndef ARGUSAGENT_CORE_EXPORTER_SCHEDULER_H
#define ARGUSAGENT_CORE_EXPORTER_SCHEDULER_H

#include <memory>
#include <map>
#include <list>
#include <atomic>

#include "SchedulerT.h"

#include "common/Common.h"
#include "common/ThreadWorker.h"
#include "common/TimePeriod.h"
#include "common/ChronoLiteral.h"

#include "TaskManager.h"
#include "exporter_metric.h"
#include "ali_metric.h"

namespace common {
    struct HttpRequest;
    struct HttpResponse;
}

namespace argus {
    struct ExporterSchedulerState : SchedulerState {
        // std::string name;
        // long errorTimes = 0;
        // long errorCount = 0;
        // long runTimes = 0;
        // std::chrono::system_clock::time_point lastBegin;
        // std::chrono::system_clock::time_point lastFinish;
        // std::chrono::system_clock::time_point lastError;
        // std::chrono::microseconds lastExecuteTime{0};
        // std::chrono::system_clock::time_point lastTrueBegin;
        // long skipTimes = 0;
        // long skipCount = 0;
        // int addStatus = 0;
        // int type = 0;
        // std::atomic<bool> isRunning{false};
        // std::string target;
        // std::string metricsPath;
        // std::chrono::seconds timeout{5};
        //this is used for aliMetric;
        // std::string springBootUrl;
        // std::string javaUrl;
        // std::string aliMetricUrl;
        // bool lastAlimetricStatus = true;
        std::shared_ptr<BaseParseMetric> pExporterMetric;
        std::shared_ptr<BaseParseMetric> pExporterMetricStatus;
        // std::vector<std::pair<std::string, std::string>> outputVector;

        ExporterItem item;

        ExporterSchedulerState() = default;
        ExporterSchedulerState(const ExporterItem &, const std::chrono::milliseconds &);

        virtual bool collect(const std::function<int (const common::HttpRequest &, common::HttpResponse &)> &httpCurl,
                             int &code, std::string &errorMsg, std::string &result) = 0;

        std::string getUrl() const;

        std::string getName() const override {
            return item.name;
        }

        std::chrono::milliseconds interval() const override {
            return item.interval;
        }
    };

    struct PrometheusCollector : ExporterSchedulerState {
        PrometheusCollector(const ExporterItem &r, const std::chrono::milliseconds &f);
        bool collect(const std::function<int (const common::HttpRequest &, common::HttpResponse &)> &httpCurl,
                     int &code, std::string &errorMsg, std::string &result) override;
    };

    struct AliMetricCollector : ExporterSchedulerState {
        std::string knownUrl[2];
        size_t activeIndex = 0;
        // std::string springBootUrl;
        // std::string javaUrl;
        // std::string aliMetricUrl;

        AliMetricCollector(const ExporterItem &r, const std::chrono::milliseconds &f);

        bool collect(const std::function<int (const common::HttpRequest &, common::HttpResponse &)> &httpCurl,
                     int &code, std::string &errorMsg, std::string &result) override;
    };

#include "common/test_support"
class ExporterScheduler : public SchedulerT<ExporterItem, ExporterSchedulerState> { // common::ThreadWorker {
public:
    ExporterScheduler();
    ~ExporterScheduler() override;

    void GetStatus(common::CommonMetric &m) const {
        SchedulerT::GetStatus("exporter_status", m);
    }

#define PROPERTY(PropertyName, VAR)                                                \
    void set##PropertyName(const std::shared_ptr<std::vector<ExporterItem>> &r) {  \
        {                                                                          \
            std::lock_guard<std::mutex> lock(mutex);                               \
            VAR = r;                                                               \
        }                                                                          \
        setItems(mergeExporterItems());                                            \
    }                                                                              \
    std::shared_ptr<std::vector<ExporterItem>> get ## PropertyName() const {       \
        std::lock_guard<std::mutex> lock(mutex);                                   \
        return VAR;                                                                \
    }

    PROPERTY(ExporterItems, mExporterItems)
    PROPERTY(ShennongExporterItems, mShennongExporterItems)

#undef PROPERTY

    static std::string BuildAddStatus(const std::string &name, int code, const std::string &msg, size_t metricNum);
    // VIRTUAL bool AliMetricExporterGet(ExporterSchedulerState &state,
    //                                   int &code, std::string &errorMsg, std::string &result) const;
    // VIRTUAL bool PrometheusGet(const ExporterSchedulerState &state,
    //                            int &code, std::string &errorMsg, std::string &result) const;

private:
    static size_t size(const std::shared_ptr<std::vector<ExporterItem>> &p) {
        return p ? p->size() : 0;
    }

    std::shared_ptr<std::map<std::string, ExporterItem>> mergeExporterItems() const;

private:
    std::shared_ptr<ExporterSchedulerState> createScheduleItem(const ExporterItem &s) const override;
    int scheduleOnce(ExporterSchedulerState &) override;

private:
    mutable std::mutex mutex;
    std::chrono::milliseconds factor = 360_s;
    // std::vector<ExporterItem> mItems;
    std::shared_ptr<std::vector<ExporterItem>> mExporterItems;
    std::shared_ptr<std::vector<ExporterItem>> mShennongExporterItems;
    // std::chrono::microseconds mScheduleInterval{100 * 1000};
    // apr_pool_t *mPool;
    // apr_thread_pool_t *mThreadPool;
    // std::shared_ptr<ThreadPool> mThreadPool;
    // common::InstanceLock mStateMapLock;
    // std::map<std::string, std::shared_ptr<ExporterSchedulerState>> mExporterSchedulerStateMap;
    std::function<int (const common::HttpRequest &, common::HttpResponse &)> httpCurl;
};
#include "common/test_support"

}
#endif //ARGUSAGENT_EXPORTERSCHEDULER_H
