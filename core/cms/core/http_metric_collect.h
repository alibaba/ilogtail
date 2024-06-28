#ifndef _HTTP_METRIC_COLLECT_H_
#define _HTTP_METRIC_COLLECT_H_

#include <list>
#include <memory>

#include "common/ThreadWorker.h"
#include "common/InstanceLock.h"
#include "common/Singleton.h"

#include "exporter_metric.h"
#include "TaskManager.h"

namespace argus {
    class HttpMetricListen;
    class HttpMetricClient;

#include "common/test_support"
class HttpMetricCollect : public common::ThreadWorker {
public:
    HttpMetricCollect();
    ~HttpMetricCollect() override;
    void AddHttpMetricRequest(const std::shared_ptr<HttpMetricClient> &);
    void Start();
private:
    void doRun() override;
    bool ParseHeader(const std::string &header, std::string &type, std::map<std::string, std::string> &tagMap,
                     std::string &errorMsg);
private:
    int mMaxConnectNum = 100;
    std::list<std::weak_ptr<HttpMetricClient>> mList;
    common::InstanceLock mListLock;
    std::shared_ptr<ExporterMetric> mMetrics;
    std::shared_ptr<ExporterMetric> mShennong;
    // ExporterMetric *mShennong;
    // HttpReceiveItem mMetricsItem;
    HttpReceiveItem mShennongItem;
    std::weak_ptr<HttpMetricListen> mListen;
};
#include "common/test_support"

    typedef common::Singleton<HttpMetricCollect> SingletonHttpMetricCollect;
}
#endif