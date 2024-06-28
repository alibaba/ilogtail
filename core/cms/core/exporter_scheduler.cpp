// #include "exporter_scheduler.h"
//
// #include "common/Config.h"
// #include "common/StringUtils.h"
// #include "common/HttpClient.h"
// #include "common/Logger.h"
// #include "common/ThreadPool.h"
// #include "common/TimeProfile.h"
// #include "common/ResourceConsumptionRecord.h"
//
// #include "argus_manager.h"
//
// using namespace std;
// using namespace common;
//
// namespace argus {
//     void ExporterSchedulerState::Collect() {
//         defer(isRunning = false);
//
//         int code = -1;
//         string errorMsg;
//         bool isOk = false;
//         string result;
//         if (this->type == 0) {
//             isOk = ExporterScheduler::PrometheusExporterGet(this, code, errorMsg, result);
//         } else if (this->type == 1) {
//             isOk = ExporterScheduler::AliMetricExporterGet(this, code, errorMsg, result);
//         } else {
//             LogInfo("skip invalid exporter-type with {}", this->name.c_str());
//         }
//
//         auto now = std::chrono::system_clock::now();
//         vector<CommonMetric> commonMetrics;
//         if (!isOk) {
//             this->errorTimes++;
//             this->errorCount++;
//             this->lastError = now;
//         } else {
//             this->pExporterMetric->AddMetric(result);
//             this->pExporterMetric->GetCommonMetrics(commonMetrics);
//         }
//         if (this->addStatus) {
//             string exporterStatus = ExporterScheduler::BuildAddStatus(this->name, code, errorMsg, commonMetrics.size());
//             LogDebug("will add exporter status for {} ", this->name.c_str());
//             this->pExporterMetricStatus->AddMetric(exporterStatus);
//             std::vector<CommonMetric> tmpCommonMetrics;
//             this->pExporterMetricStatus->GetCommonMetrics(tmpCommonMetrics);
//             // for (auto it = tmpCommonMetrics.begin(); it != tmpCommonMetrics.end(); it++) {
//             //     commonMetrics.push_back(*it);
//             // }
//             commonMetrics.insert(commonMetrics.end(), tmpCommonMetrics.begin(), tmpCommonMetrics.end());
//         }
//         if (!commonMetrics.empty()) {
//             if (auto pManager = SingletonArgusManager::Instance()->getChannelManager()) {
//                 pManager->SendMetricsToNextModule(commonMetrics, this->outputVector);
//             }
//             LogDebug("send metrics {} to outputChannel", commonMetrics.size());
//         }
//         // this->lastFinish = now;
//         this->lastExecuteTime = std::chrono::duration_cast<std::chrono::microseconds>(now - this->lastTrueBegin);
//         this->lastFinish = now;
//     }
//
//     string ExporterScheduler::BuildAddStatus(const string &name, int code, const string &msg, size_t metricNum) {
//         string exportStatus = string("__argus_exporter_status__{__argus_exporter_name__=\"") + name +
//                               "\",__argus_exporter_code__=\"" + StringUtils::NumberToString(code) +
//                               "\",__argus_exporter_error_msg__=\"" + msg + "\"} " +
//                               StringUtils::NumberToString(metricNum);
//         return exportStatus;
//     }
//
//     bool ExporterScheduler::PrometheusExporterGet(ExporterSchedulerState *state,
//                                                   int &code, string &errorMsg, string &result) {
//         using namespace std::chrono;
//         HttpRequest httpRequest;
//         httpRequest.url = state->target + state->metricsPath;
//         httpRequest.timeout = static_cast<int>(duration_cast<seconds>(state->timeout).count());
//         HttpResponse httpResponse;
//         bool isOk = true;
//         if (HttpClient::HttpCurl(httpRequest, httpResponse) != 0) {
//             //curl发生错误
//             LogWarn("get metrics from {} error={}", httpRequest.url.c_str(),
//                      httpResponse.errorMsg.c_str());
//             isOk = false;
//             errorMsg = httpResponse.errorMsg;
//
//         } else if (httpResponse.resCode != 200) {
//             //相应不对
//             isOk = false;
//             LogWarn("get metrics from {} with error response,responseCode={},errorInfo={},response={}",
//                      httpRequest.url.c_str(), httpResponse.resCode, httpResponse.errorMsg.c_str(),
//                      httpResponse.result.c_str());
//             errorMsg = "http response code is not 200";
//             code = httpResponse.resCode;
//         } else {
//             //正确
//             errorMsg = "ok";
//             code = 200;
//             result = httpResponse.result;
//             LogDebug("get metrics from {}:{}", httpRequest.url.c_str(), httpResponse.result.c_str());
//         }
//         return isOk;
//     }
//
//     bool ExporterScheduler::AliMetricExporterGet(ExporterSchedulerState *state,
//                                                  int &code, string &errorMsg, string &result) {
//         if (!state->lastAlimetricStatus) {
//             if (state->aliMetricUrl == state->springBootUrl) {
//                 state->aliMetricUrl = state->javaUrl;
//             } else if (state->aliMetricUrl == state->javaUrl) {
//                 state->aliMetricUrl = state->springBootUrl;
//             }
//         }
//         bool isOk = true;
//         HttpRequest httpRequest;
//         httpRequest.url = state->aliMetricUrl;
//         namespace chrono = std::chrono;
//         httpRequest.timeout = static_cast<int>(chrono::duration_cast<chrono::seconds>(state->timeout).count());
//         HttpResponse httpResponse;
//         if (HttpClient::HttpCurl(httpRequest, httpResponse) != 0) {
//             //curl发生错误
//             LogWarn( "get metrics from {} error={}", httpRequest.url, httpResponse.errorMsg);
//             isOk = false;
//             errorMsg = httpResponse.errorMsg;
//             state->lastAlimetricStatus = false;
//         } else if (httpResponse.resCode != 200) {
//             //相应不对
//             isOk = false;
//             LogWarn( "get metrics from {} with error response,responseCode={},errorInfo={},response={}",
//                      httpRequest.url, httpResponse.resCode, httpResponse.errorMsg, httpResponse.result);
//             errorMsg = "http response code is not 200";
//             code = httpResponse.resCode;
//             state->lastAlimetricStatus = false;
//         } else {
//             //正确
//             errorMsg = "ok";
//             code = 200;
//             state->lastAlimetricStatus = true;
//             result = httpResponse.result;
//             LogDebug("get metrics from {}:{}", httpRequest.url.c_str(), httpResponse.result.c_str());
//         }
//         return isOk;
//     }
//
//     ExporterScheduler::ExporterScheduler() {
//         // apr_pool_create(&mPool, NULL);
//         Config *cfg = SingletonConfig::Instance();
//         mScheduleInterval = std::chrono::microseconds{
//             cfg->GetValue("agent.exporter.schedule.interval", int64_t(100*1000))};
//         int threadNum = cfg->GetValue("agent.exporter.schedule.thread.number", 5);
//         int maxThreadNum = cfg->GetValue("agent.exporter.schedule.max.thread.number", 10);
//         // apr_thread_pool_create(&mThreadPool,threadNum,maxThreadNum,mPool);
//         mThreadPool = ThreadPool::Option{}.min(threadNum).max(maxThreadNum).name("ExporterScheduler").makePool();
//     }
//
//     ExporterScheduler::~ExporterScheduler() {
//         endThread();
//         join();
//         // apr_thread_pool_destroy(mThreadPool);
//         // apr_pool_destroy(mPool);
//     }
//
//     void ExporterScheduler::doRun() {
//         TaskManager *pTask = SingletonTaskManager::Instance();
//
//         std::shared_ptr<vector<ExporterItem>> prevExporterItems;
//         std::shared_ptr<vector<ExporterItem>> prevShennongExporterItems;
//         for (; !isThreadEnd(); sleepFor(mScheduleInterval)) {
//             TimeProfile tp;
//             defer(RecordResourceConsumption("ExporterScheduler::doRun", tp.cost()));
//
//             bool isChanged = pTask->ExporterItems().Get(prevExporterItems);
//             if (isChanged) {
//                 mExporterItems = *prevExporterItems;
//                 LogDebug("ExporterItems changed:{}", mExporterItems.size());
//             }
//             // bool isChanged = false;
//             // if(pTask->ExporterItemsChanged())
//             // {
//             //     mExporterItems.clear();
//             //     pTask->GetExporterItems(mExporterItems,true);
//             //     isChanged = true;
//             //     LogDebug("ExporterItems changed:{}", mExporterItems.size());
//             // }
//             // if (pTask->ShennongExporterItemsChanged())
//             // {
//             //     mShennongExporterItems.clear();
//             //     pTask->GetShennongExporterItems(mShennongExporterItems, true);
//             //     isChanged = true;
//             //     LogDebug("ShennongExporterItems changed:{}", mShennongExporterItems.size());
//             // }
//             if (pTask->ShennongExporterItems().Get(prevShennongExporterItems)) {
//                 isChanged = true;
//                 mShennongExporterItems = *prevShennongExporterItems;
//                 LogDebug("ShennongExporterItems changed:{}", mShennongExporterItems.size());
//             }
//             if (isChanged) {
//                 mItems = mExporterItems;
//                 mItems.insert(mItems.end(), mShennongExporterItems.begin(), mShennongExporterItems.end());
//                 if (mItems.size() != mExporterItems.size() + mShennongExporterItems.size()) {
//                     LogWarn( "m_scriptItems and m_aliScriptItems conflicts! Some aliScript items may be discarded!");
//                 }
//             }
//             for (auto & mItem : mItems) {
//                 Schedule(mItem);
//             }
//             if (mItems.size() != mExporterSchedulerStateMap.size()) {
//                 std::lock_guard<InstanceLock> lock(mStateMapLock);
//                 for (auto itMap = mExporterSchedulerStateMap.begin(); itMap != mExporterSchedulerStateMap.end();) {
//                     if (itMap->second->isRunning) {
//                         itMap++;
//                     } else {
//                         bool needDelete = true;
//                         for (auto & mItem : mItems) {
//                             if (mItem.mid == itMap->first) {
//                                 needDelete = false;
//                                 break;
//                             }
//                         }
//                         if (needDelete) {
//                             // delete itMap->second;
//                             itMap = mExporterSchedulerStateMap.erase(itMap);
//
//                         } else {
//                             itMap++;
//                         }
//                     }
//                 }
//             }
//         }
//     }
//
//     void ExporterScheduler::Schedule(ExporterItem &item) {
//         auto it = mExporterSchedulerStateMap.find(item.mid);
//         if (it != mExporterSchedulerStateMap.end()) {
//             CheckRun(item, it->second);
//         } else {
//             CheckFirstRun(item);
//         }
//     }
//
//     void ExporterScheduler::CheckFirstRun(ExporterItem &item) {
//         auto pState = std::make_shared<ExporterSchedulerState>();
//
//         auto nowTime = std::chrono::system_clock::now();
//         auto t0 = getHashStartTime(nowTime, item.interval, 360_s) - item.interval;
//         pState->lastBegin = t0;
//         pState->lastFinish = t0;
//         pState->isRunning = false;
//         pState->name = item.name;
//         pState->target = item.target;
//         pState->metricsPath = item.metricsPath;
//         pState->timeout = item.timeout;
//         pState->outputVector = item.outputVector;
//         pState->addStatus = item.addStatus;
//         pState->type = item.type;
//         if (pState->type == 0) {
//             pState->pExporterMetric = make_shared<ExporterMetric>(item.metricFilterInfos, item.labelAddInfos);
//         } else {
//             pState->pExporterMetric = make_shared<AliMetric>(item.metricFilterInfos, item.labelAddInfos);
//             size_t index1 = pState->target.find("://");
//             if (index1 != string::npos) {
//                 pState->target = pState->target.substr(index1 + 3);
//             }
//             size_t index2 = pState->target.find(':');
//             if (index2 != string::npos) {
//                 pState->target = pState->target.substr(0, index2);
//             }
//             pState->javaUrl = pState->target + ":8006/metrics";
//             pState->springBootUrl = pState->target + ":7002/alimetrics";
//             pState->aliMetricUrl = pState->springBootUrl;
//             pState->lastAlimetricStatus = true;
//         }
//
//         if (pState->addStatus) {
//             vector<MetricFilterInfo> emptyMetricFilterInfo;
//             if (pState->type == 0) {
//                 pState->pExporterMetricStatus = make_shared<ExporterMetric>(emptyMetricFilterInfo, item.labelAddInfos);
//             } else {
//                 pState->pExporterMetricStatus = make_shared<AliMetric>(emptyMetricFilterInfo, item.labelAddInfos);
//             }
//         }
//         mStateMapLock.lock();
//         mExporterSchedulerStateMap[item.mid] = pState;
//         mStateMapLock.unlock();
//     }
//
//     void ExporterScheduler::CheckRun(ExporterItem &item, const std::shared_ptr<ExporterSchedulerState> &state) const {
//         auto nowTime = std::chrono::system_clock::now();
//         auto t1 = item.interval;
//         auto t2 = nowTime - state->lastBegin;
//         if (-1 * t1 < t2 && t2 < t1) {
//             return;
//         }
//         if (state->isRunning) {
//             return;
//         }
//         int64_t n;
//         if (nowTime > state->lastBegin) {
//             n = (nowTime - state->lastBegin) / item.interval;
//             state->lastBegin += n * item.interval;
//         } else {
//             n = (state->lastBegin - nowTime) / item.interval;
//             state->lastBegin -= n * item.interval;
//         }
//         if (n > 1) {
//             state->skipTimes++;
//             state->skipCount++;
//             LogWarn( "exporter item({}) loss schedule {} times", item.target, n - 1);
//         }
//         state->lastTrueBegin = nowTime;
//         state->runTimes++;
//         state->isRunning = true;
//         // apr_thread_pool_push(mThreadPool,ExporterScheduler::ThreadCollectFunc,(void *)state,APR_THREAD_TASK_PRIORITY_NORMAL, (void *) NULL);
//         mThreadPool->commit(item.target, [state]() { state->Collect(); });
//     }
//
//     void ExporterScheduler::GetStateMap(map<string, std::shared_ptr<ExporterSchedulerState>> &stateMap) {
//         mStateMapLock.lock();
//         stateMap = mExporterSchedulerStateMap;
//         mStateMapLock.unlock();
//     }
//
//     void ExporterScheduler::GetStatus(CommonMetric &metric) {
//         vector<string> errorTasks;
//         vector<string> okTasks;
//         vector<string> skipTasks;
//         mStateMapLock.lock();
//         size_t totalNumber = mExporterSchedulerStateMap.size();
//         for (auto & it : mExporterSchedulerStateMap) {
//             if (it.second->errorCount > 0) {
//                 errorTasks.push_back(it.second->name);
//             }
//             if (it.second->skipCount > 0) {
//                 skipTasks.push_back(it.second->name);
//             }
//             if (it.second->skipCount == 0 && it.second->errorCount == 0) {
//                 okTasks.push_back(it.second->name);
//             }
//             it.second->skipCount = 0;
//             it.second->errorCount = 0;
//         }
//         mStateMapLock.unlock();
//
//         int exporter_status = (!errorTasks.empty() || !skipTasks.empty()? 1: 0);
//         // if (!errorTasks.empty() || !skipTasks.empty()) {
//         //     exporter_status = 1;
//         // }
//         metric.value = exporter_status;
//         metric.name = "exporter_status";
//         metric.timestamp = NowSeconds();
//         metric.tagMap["number"] = StringUtils::NumberToString(totalNumber);
//         metric.tagMap["ok_list"] = StringUtils::join(okTasks, ",");
//         metric.tagMap["error_list"] = StringUtils::join(errorTasks, ",");
//         metric.tagMap["skip_list"] = StringUtils::join(skipTasks, ",");
//     }
// }