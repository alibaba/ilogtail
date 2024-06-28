// #ifndef _EXPORTER_SCHEDULER_H_
// #define _EXPORTER_SCHEDULER_H_
//
// #include <memory>
// #include <map>
// #include <list>
// #include <atomic>
//
// #include "common/Common.h"
// #include "common/ThreadWorker.h"
// #include "common/TimePeriod.h"
//
// #include "TaskManager.h"
// #include "exporter_metric.h"
// #include "ali_metric.h"
//
// class ThreadPool;
//
// namespace argus {
//     struct ExporterSchedulerState {
//         std::string name;
//         long errorTimes = 0;
//         long errorCount = 0;
//         long runTimes = 0;
//         std::chrono::system_clock::time_point lastBegin;
//         std::chrono::system_clock::time_point lastFinish;
//         std::chrono::system_clock::time_point lastError;
//         std::chrono::microseconds lastExecuteTime{0};
//         std::chrono::system_clock::time_point lastTrueBegin;
//         long skipTimes = 0;
//         long skipCount = 0;
//         int addStatus = 0;
//         int type = 0;
//         std::atomic<bool> isRunning{false};
//         std::string target;
//         std::string metricsPath;
//         std::chrono::seconds timeout{5};
//         //this is used for aliMetric;
//         std::string springBootUrl;
//         std::string javaUrl;
//         std::string aliMetricUrl;
//         bool lastAlimetricStatus = true;
//         std::shared_ptr<BaseParseMetric> pExporterMetric;
//         std::shared_ptr<BaseParseMetric> pExporterMetricStatus;
//         std::vector<std::pair<std::string, std::string>> outputVector;
//
//         void Collect();
//     };
//
// #include "common/test_support"
// class ExporterScheduler : public common::ThreadWorker {
// public:
//     ExporterScheduler();
//     ~ExporterScheduler() override;
//
//     void GetStateMap(std::map<std::string, std::shared_ptr<ExporterSchedulerState>> &stateMap);
//     void GetStatus(common::CommonMetric &metric);
//     static std::string BuildAddStatus(const std::string &name, int code, const std::string &msg, size_t metricNum);
//     // static void *APR_THREAD_FUNC ThreadCollectFunc(apr_thread_t *thread, void *para);
//     static bool AliMetricExporterGet(ExporterSchedulerState *state,
//                                      int &code, std::string &errorMsg, std::string &result);
//     static bool PrometheusExporterGet(ExporterSchedulerState *state,
//                                       int &code, std::string &errorMsg, std::string &result);
// private:
//     void doRun() override;
//     void Schedule(ExporterItem &item);
//     void CheckFirstRun(ExporterItem &item);
//     void CheckRun(ExporterItem &item, const std::shared_ptr<ExporterSchedulerState> &state) const;
// private:
//     std::vector<ExporterItem> mItems;
//     std::vector<ExporterItem> mExporterItems;
//     std::vector<ExporterItem> mShennongExporterItems;
// 	std::chrono::microseconds mScheduleInterval{100 * 1000};
//     // apr_pool_t *mPool;
//     // apr_thread_pool_t *mThreadPool;
//     std::shared_ptr<ThreadPool> mThreadPool;
//     common::InstanceLock mStateMapLock;
//     std::map<std::string, std::shared_ptr<ExporterSchedulerState>> mExporterSchedulerStateMap;
// };
// #include "common/test_support"
//
// }
// #endif
