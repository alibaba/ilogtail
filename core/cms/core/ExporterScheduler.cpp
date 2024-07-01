//
// Created by 韩呈杰 on 2024/2/23.
//
#include "ExporterScheduler.h"
#include "argus_manager.h"

#include "common/TimeFormat.h"
#include "common/Common.h" // getHashStartTime, joinOutput
#include "common/HttpClient.h"
#include "common/PrometheusMetric.h"
#include "common/Chrono.h"

#include "common/Config.h"

using namespace argus;
using namespace common;

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
ExporterSchedulerState::ExporterSchedulerState(const ExporterItem &r, const std::chrono::milliseconds &factor) {
    using namespace std::chrono;
    auto nowTime = system_clock::now();
    auto steadyNow = steady_clock::now();
    // auto t0 = getHashStartTime(nowTime, item.interval, 360_s);
    item = r;
    nextTime = getHashStartTime(steadyNow, item.interval, factor);
    // if (pState->item.type == 0) {
    //     pState->pExporterMetric = std::make_shared<ExporterMetric>(item.metricFilterInfos, item.labelAddInfos);
    //     if (pState->item.addStatus) {
    //         pState->pExporterMetricStatus = std::make_shared<ExporterMetric>(item.labelAddInfos);
    //     }
    // } else {
    //     pState->pExporterMetric = std::make_shared<AliMetric>(item.metricFilterInfos, item.labelAddInfos);
    //     size_t index1 = item.target.find("://");
    //     if (index1 != std::string::npos) {
    //         pState->item.target = item.target.substr(index1 + 3);
    //     }
    //     size_t index2 = pState->item.target.find(':');
    //     if (index2 != std::string::npos) {
    //         pState->item.target = pState->item.target.substr(0, index2);
    //     }
    //     pState->javaUrl = pState->item.target + ":8006/metrics";
    //     pState->springBootUrl = pState->item.target + ":7002/alimetrics";
    //     pState->aliMetricUrl = pState->springBootUrl;
    //     // pState->lastAlimetricStatus = true;
    //     if (pState->item.addStatus) {
    //         pState->pExporterMetricStatus = std::make_shared<AliMetric>(item.labelAddInfos);
    //     }
    // }

    auto delay = duration_cast<system_clock::duration>(nextTime - steadyNow);
    LogInfo("first run exporter <{}>, outputVector: {}, mid: {}, do hash to {:L}, delay: {:.3f}ms, interval={}",
            getName(), joinOutput(item.outputVector), item.mid, nowTime + delay,
            duration_cast<fraction_millis>(delay).count(), item.interval);
}

std::string ExporterSchedulerState::getUrl() const {
    return item.target + item.metricsPath;
}

static bool doHttpCurl(const std::function<int(const common::HttpRequest &, common::HttpResponse &)> &httpCurl,
                       const std::string &url, const std::chrono::seconds &timeout,
                       const std::vector<std::string> &headers,
                       int &code, std::string &errorMsg, std::string &result) {
    HttpRequest request;
    request.url = url;
    request.timeout = static_cast<int>(timeout.count());
    request.httpHeader = headers;

    HttpResponse response;
    // bool isOk = true;
    if (httpCurl(request, response) != HttpClient::Success) {
        //curl发生错误
        LogWarn("get metrics from {} error={}, resCode={}", request.url, response.errorMsg, response.resCode);
        // isOk = false;
        // errorMsg = response.errorMsg;
    } else if (response.resCode != 200) {
        //相应不对
        // isOk = false;
        LogWarn("get metrics from {} with error response,responseCode={},errorInfo={},response={}",
                request.url, response.resCode, response.errorMsg, response.result);
        response.errorMsg = (response.errorMsg.empty() ? "http response code is not 200" : response.errorMsg);
        // code = response.resCode;
    } else {
        //正确
        // errorMsg = "ok";
        // code = 200;
        // result = response.result;
        LogDebug("get metrics from {}:{}", request.url, response.result);
    }

    code = response.resCode;
    errorMsg = response.errorMsg;
    result = response.result;

    return response.resCode == (int) HttpStatus::OK;
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
PrometheusCollector::PrometheusCollector(const ExporterItem &r, const std::chrono::milliseconds &f)
        : ExporterSchedulerState(r, f) {
    pExporterMetric = std::make_shared<ExporterMetric>(item.metricFilterInfos, item.labelAddInfos);
    if (item.addStatus) {
        pExporterMetricStatus = std::make_shared<ExporterMetric>(item.labelAddInfos);
    }
}

bool PrometheusCollector::collect(
        const std::function<int(const common::HttpRequest &, common::HttpResponse &)> &httpCurl,
        int &code, std::string &errorMsg, std::string &result) {
    return doHttpCurl(httpCurl, getUrl(), item.timeout, item.headers, code, errorMsg, result);
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
AliMetricCollector::AliMetricCollector(const ExporterItem &r, const std::chrono::milliseconds &f)
        : ExporterSchedulerState(r, f) {
    pExporterMetric = std::make_shared<AliMetric>(item.metricFilterInfos, item.labelAddInfos);
    if (item.addStatus) {
        pExporterMetricStatus = std::make_shared<AliMetric>(item.labelAddInfos);
    }

    size_t index1 = item.target.find("://");
    if (index1 != std::string::npos) {
        item.target = item.target.substr(index1 + 3);
    }
    size_t index2 = item.target.find(':');
    if (index2 != std::string::npos) {
        item.target = item.target.substr(0, index2);
    }

    knownUrl[0] = item.target + ":7002/alimetrics";
    knownUrl[1] = item.target + ":8006/metrics";
    activeIndex = 0;
    // javaUrl = item.target + ":8006/metrics";
    // springBootUrl = item.target + ":7002/alimetrics";
    // aliMetricUrl = springBootUrl;
    // pState->lastAlimetricStatus = true;
}

bool AliMetricCollector::collect(const std::function<int(const HttpRequest &, HttpResponse &)> &httpCurl,
                                 int &code, std::string &errorMsg, std::string &result) {
    bool isOk = doHttpCurl(httpCurl, knownUrl[activeIndex], item.timeout, item.headers, code, errorMsg, result);
    if (!isOk) {
        activeIndex = (activeIndex + 1) % (sizeof(knownUrl) / sizeof(knownUrl[0]));
    }

    return isOk;
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///

ExporterScheduler::ExporterScheduler() : SchedulerT<ExporterItem, ExporterSchedulerState>() {
    httpCurl = HttpClient::HttpCurl;

    Config *cfg = SingletonConfig::Instance();
    // mScheduleInterval = std::chrono::microseconds{
    //         cfg->GetValue("agent.exporter.schedule.interval", int64_t(100 * 1000))};
    int threadNum = cfg->GetValue("agent.exporter.schedule.thread.number", 5);
    int maxThreadNum = cfg->GetValue("agent.exporter.schedule.max.thread.number", 10);
    // apr_thread_pool_create(&mThreadPool,threadNum,maxThreadNum,mPool);
    setThreadPool(ThreadPool::Option{}.min(threadNum).max(maxThreadNum).name("ExporterScheduler").makePool());
}

ExporterScheduler::~ExporterScheduler() {
    close();
}

std::shared_ptr<ExporterSchedulerState> ExporterScheduler::createScheduleItem(const ExporterItem &item) const {
    if (item.type == 0) {
        return std::make_shared<PrometheusCollector>(item, factor);
    }
    return std::make_shared<AliMetricCollector>(item, factor);
    // auto pState = std::make_shared<ExporterSchedulerState>();
    //
    // auto nowTime = std::chrono::system_clock::now();
    // auto steadyNow = std::chrono::steady_clock::now();
    // // auto t0 = getHashStartTime(nowTime, item.interval, 360_s);
    // pState->nextTime = getHashStartTime(steadyNow, item.interval, factor);
    // pState->item = item;
    // if (pState->item.type == 0) {
    //     pState->pExporterMetric = std::make_shared<ExporterMetric>(item.metricFilterInfos, item.labelAddInfos);
    //     if (pState->item.addStatus) {
    //         pState->pExporterMetricStatus = std::make_shared<ExporterMetric>(item.labelAddInfos);
    //     }
    // } else {
    //     pState->pExporterMetric = std::make_shared<AliMetric>(item.metricFilterInfos, item.labelAddInfos);
    //     size_t index1 = item.target.find("://");
    //     if (index1 != std::string::npos) {
    //         pState->item.target = item.target.substr(index1 + 3);
    //     }
    //     size_t index2 = pState->item.target.find(':');
    //     if (index2 != std::string::npos) {
    //         pState->item.target = pState->item.target.substr(0, index2);
    //     }
    //     pState->javaUrl = pState->item.target + ":8006/metrics";
    //     pState->springBootUrl = pState->item.target + ":7002/alimetrics";
    //     pState->aliMetricUrl = pState->springBootUrl;
    //     // pState->lastAlimetricStatus = true;
    //     if (pState->item.addStatus) {
    //         pState->pExporterMetricStatus = std::make_shared<AliMetric>(item.labelAddInfos);
    //     }
    // }
    //
    // auto delay = pState->nextTime - steadyNow;
    // LogInfo("first run exporter <{}>, outputVector: {}, mid: {}, do hash to {:L}, delay: {:.3f}ms, interval={}",
    //         pState->getName(), joinOutput(pState->item.outputVector), pState->item.mid, nowTime + delay,
    //         std::chrono::duration_cast<std::chrono::fraction_millis>(delay).count(), pState->item.interval);
    //
    // return pState;
}

int ExporterScheduler::scheduleOnce(ExporterSchedulerState &r) {
    int code = -1;
    std::string errorMsg;
    std::string result;
    bool isOk = r.collect(httpCurl, code, errorMsg, result);
    // if (nullptr != dynamic_cast<ExporterMetric *>(r.pExporterMetric.get())) {
    //     isOk = PrometheusGet(*r, code, errorMsg, result);
    // } else if (nullptr != dynamic_cast<AliMetric *>(r.pExporterMetric.get())) {
    //     isOk = AliMetricExporterGet(*r, code, errorMsg, result);
    // } else {
    //     LogInfo("skip invalid exporter-type with {}", r.item.name);
    // }

    // auto now = std::chrono::system_clock::now();
    std::vector<CommonMetric> commonMetrics;
    if (isOk) {
        r.pExporterMetric->AddMetric(result);
        r.pExporterMetric->GetCommonMetrics(commonMetrics);
    }
    if (r.pExporterMetricStatus) {
        std::string exporterStatus = BuildAddStatus(r.item.name, code, errorMsg, commonMetrics.size());
        LogDebug("will add exporter status for {}", r.item.name);
        r.pExporterMetricStatus->AddMetric(exporterStatus);
        std::vector<CommonMetric> tmpCommonMetrics;
        r.pExporterMetricStatus->GetCommonMetrics(tmpCommonMetrics);
        // for (auto it = tmpCommonMetrics.begin(); it != tmpCommonMetrics.end(); it++) {
        //     commonMetrics.push_back(*it);
        // }
        commonMetrics.insert(commonMetrics.end(), tmpCommonMetrics.begin(), tmpCommonMetrics.end());
    }
    if (!commonMetrics.empty()) {
        if (auto pManager = SingletonArgusManager::Instance()->getChannelManager()) {
            pManager->SendMetricsToNextModule(commonMetrics, r.item.outputVector);
        }
        LogDebug("send metrics {} to outputChannel", commonMetrics.size());
    }
    // this->lastFinish = now;
    // this->lastExecuteTime = std::chrono::duration_cast<std::chrono::microseconds>(now - this->lastTrueBegin);
    // this->lastFinish = now;

    return 0;
}

std::shared_ptr<std::map<std::string, ExporterItem>> ExporterScheduler::mergeExporterItems() const {
    auto merged = std::make_shared<std::map<std::string, ExporterItem>>();

    std::lock_guard<std::mutex> lock(mutex);

    std::vector<ExporterItem> *arr[] = {
            mExporterItems.get(),
            mShennongExporterItems.get(),
    };
    for (auto const *items: arr) {
        if (items) {
            for (const auto &s: *items) {
                merged->emplace(s.mid, s);
            }
        }
    }

    return merged;
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
std::string ExporterScheduler::BuildAddStatus(const std::string &name, int code, const std::string &msg, size_t val) {
    std::string exportStatus = fmt::format(
            "__argus_exporter_status__{{"
            "__argus_exporter_name__=\"{}\","
            "__argus_exporter_code__=\"{}\","
            "__argus_exporter_error_msg__=\"{}\"}} {}",
            _PromEsc(name), code, _PromEsc(msg), val);
    return exportStatus;
}
//
// bool ExporterScheduler::PrometheusGet(const ExporterSchedulerState &state,
//                                       int &code, std::string &errorMsg, std::string &result) const {
//     return doHttpCurl(httpCurl, state.getUrl(), state.item.timeout, code, errorMsg, result);
// }
//
// bool ExporterScheduler::AliMetricExporterGet(ExporterSchedulerState &state,
//                                              int &code, std::string &errorMsg, std::string &result) const {
//     bool isOk = doHttpCurl(httpCurl, state.aliMetricUrl, state.item.timeout, code, errorMsg, result);
//     // bool isOk = true;
//     // HttpRequest httpRequest;
//     // httpRequest.url = state.aliMetricUrl;
//     //
//     // using namespace std::chrono;
//     // httpRequest.timeout = static_cast<int>(duration_cast<seconds>(state.item.timeout).count());
//     // HttpResponse httpResponse;
//     // if (httpCurl(httpRequest, httpResponse) != 0) {
//     //     //curl发生错误
//     //     LogWarn("get metrics from {} error={}", httpRequest.url, httpResponse.errorMsg);
//     //     isOk = false;
//     //     errorMsg = httpResponse.errorMsg;
//     //     // state.lastAlimetricStatus = false;
//     // } else if (httpResponse.resCode != 200) {
//     //     //相应不对
//     //     isOk = false;
//     //     LogWarn("get metrics from {} with error response,responseCode={},errorInfo={},response={}",
//     //             httpRequest.url, httpResponse.resCode, httpResponse.errorMsg, httpResponse.result);
//     //     errorMsg = "http response code is not 200";
//     //     code = httpResponse.resCode;
//     //     // state.lastAlimetricStatus = false;
//     // } else {
//     //     //正确
//     //     errorMsg = "ok";
//     //     code = 200;
//     //     // state.lastAlimetricStatus = true;
//     //     result = httpResponse.result;
//     //     LogDebug("get metrics from {}:{}", httpRequest.url.c_str(), httpResponse.result.c_str());
//     // }
//
//     if (!isOk) {
//         state.aliMetricUrl = (state.aliMetricUrl == state.springBootUrl ? state.javaUrl : state.springBootUrl);
//     }
//
//     return isOk;
// }
