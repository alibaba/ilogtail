#include "cloud_channel.h"

#include "common/Config.h"
#include "common/Logger.h"
#include "common/StringUtils.h"
#include "common/HttpClient.h"
#include "common/Gzip.h"
#include "common/PrometheusMetric.h"
#include "common/Md5.h"
#include "common/SystemUtil.h"
#include "common/TimeProfile.h"
#include "common/JsonValueEx.h"
#include "common/ResourceConsumptionRecord.h"
#include "common/Chrono.h"
#include "common/ChronoLiteral.h"
#include "common/FileUtils.h"

#include "collect_info.h"
#include "cloud_signature.h"

using namespace std;
using namespace argus;
using namespace common;
using namespace cloudMonitor;

const char *const CloudChannel::Name = "cloudMonitor";

CloudChannel::CloudChannel(const std::function<CloudChannel::THttpPost> &post) : fnHttpPost(post) {
    mInterval = std::chrono::seconds{SingletonConfig::Instance()->GetValue("cms.agent.metric.interval", 15)};
    SingletonConfig::Instance()->GetValue("cms.agent.max.metric.size", mMaxMetricSize, 10000);
    SingletonConfig::Instance()->GetValue("cms.agent.metric.send.size", mMetricSendSize, 2000);
    mMaxMsgSize = 200;
    mHostname = SystemUtil::getHostname();
    mMainIp = SystemUtil::getMainIpInCloud();
}

CloudChannel::CloudChannel() : CloudChannel(HttpClient::HttpPost) {
}

void CloudChannel::Start() {
    this->runIt();
}

CloudChannel::~CloudChannel() {
    LogInfo("the cloudChannel will exit!");
    endThread();
    join();
}

NodeItem CloudChannel::nodeItem() const {
    std::lock_guard<std::mutex> guard(this->mutex);
    return mNodeItem;
}

int CloudChannel::addMessage(const std::string &name, const std::chrono::system_clock::time_point &timestamp,
                             int exitCode, const std::string &result, const std::string &conf,
                             bool reportStatus, const std::string &mid) {
    CloudMsg cloudMsg;
    cloudMsg.name = name;
    cloudMsg.timestamp = std::chrono::Now<ByMillis>();
    cloudMsg.msg = result;

    bool exceeded;
    Sync(mMsgs) {
        mMsgs.push_back(cloudMsg);
        exceeded = mMsgs.size() > mMaxMsgSize;
        if (exceeded) {
            mMsgs.pop_front();
        }
    }}}
    if (exceeded) {
        LogWarn("drop msg while the size of msgQueue exceeds the maxSize({}). agent: {}", mMaxMsgSize, nodeItem().str());
    }
    return 0;
}

int CloudChannel::AddCommonMetrics(const vector<CommonMetric> &metrics, std::string conf) {
    if (metrics.empty()) {
        return -1;
    }
    CloudMetricConfig metricConfig;
    if (!ParseCloudMetricConfig(conf, metricConfig)) {
        LogWarn("skip metrics with invalid channel-conf({})", conf.c_str());
        return -2;
    }
    NodeItem nodeItem;
    SingletonTaskManager::Instance()->GetNodeItem(nodeItem);
    if (nodeItem.instanceId.empty()) {
        LogInfo("wait heartbeat response for ok, as the instanceId is empty");
        return -3;
    }
    vector<string> bodyVector;
    GetSendMetricBody(metrics, bodyVector, metricConfig);
    for (auto &it: bodyVector) {
        SendMetric(it, metricConfig);
    }
    return 0;
}

/*
int CloudChannel::AddCommonMetrics(const vector<CommonMetric> &metrics,std::string conf)
{
    CloudMetricConfig metricConfig;
    if(!ParseCloudMetricConfig(conf,metricConfig))
    {
        LogWarn("skip metrics with invalid channel-conf({})",conf.c_str());
        return;
    }
    string key = metricConfig.acckeyId+metricConfig.accKeySecretSign+metricConfig.uploadEndpoint;
    mMetricListMapLock.lock();
    auto itList = mMetricListMap.find(key);
    if(mMetricListMap.end() == itList)
    {
        list<CommonMetric> metricList;
        mMetricListMap[key] = metricList;
        mMetricConfigMap[key] = metricConfig;
        itList = mMetricListMap.find(key);
    }
    for(auto it = metrics.begin(); it != metrics.end(); it++)
    {
        if(itList->second.size()>=mMaxMetricSize)
        {
            itList->second.pop_front();
            LogWarn("drop metric while the size of msgQueue exceeds the maxSize({})",mMaxMetricSize);
        }
        itList->second.push_back(*it);
    }
    mMetricListMapLock.unlock();
}
size_t CloudChannel::GetSendMetricBody(string &body,CloudMetricConfig &metricConfig)
{
    body = "";
    mMetricListMapLock.lock();
    for(auto it = mMetricListMap.begin();it != mMetricListMap.end();it++)
    {
        string key = it->first;
        if(mMetricListMap[key].empty())
        {
            continue;
        }
        metricConfig = mMetricConfigMap[key];
        for(int i=0;i<mMetricSendSize && mMetricListMap[key].size()>0;i++)
        {
            CommonMetric metric = mMetricListMap[key].front();
            mMetricListMap[key].pop_front();
            string line;
            PrometheusMetric::MetricToLine(metric,line);
            if(i==0)
            {
                body += line;
            }else
            {
                body += "\n"+line;
            }
        }
    }
    mMetricListMapLock.unlock();
    return body.size();
}*/
size_t CloudChannel::GetSendMetricBody(const vector<CommonMetric> &metrics, vector<string> &bodyVector,
                                       CloudMetricConfig &metricConfig) const {
    string body;
    for (size_t i = 0; i < metrics.size(); i++) {
        string line = PrometheusMetric::MetricToLine(metrics[i], metricConfig.needTimestamp);
        if (i % mMetricSendSize == 0) {
            if (!body.empty()) {
                bodyVector.push_back(body);
            }
            body = "";
        }
        // https://prometheus.io/docs/instrumenting/exposition_formats/#text-format-details
        // Prometheus' text-based format is line oriented. Lines are separated by a line feed character (\n).
        // The last line must end with a line feed character
        body.append(line).append("\n");
    }
    if (!body.empty()) {
        bodyVector.push_back(body);
    }
    return bodyVector.size();
}

void CloudChannel::doRun() {
    std::shared_ptr<vector<MetricItem>> prevMetricItems;

    std::chrono::microseconds seconds = mInterval;
    for (; !isThreadEnd(); sleepFor(seconds <= 0_us ? std::chrono::seconds{5} : seconds)) {
        TimeProfile timeProfile;
        {
            CpuProfile("CloudChannel::doRun", timeProfile.lastTime());

            {
                std::lock_guard<std::mutex> guard(this->mutex);
                SingletonTaskManager::Instance()->GetNodeItem(mNodeItem);
                SingletonTaskManager::Instance()->GetCloudAgentInfo(mCloudAgentInfo);
            }

            if (SingletonTaskManager::Instance()->MetricItems().Get(prevMetricItems)) {
                mMetricItems = *prevMetricItems;
                //更新其他统计
                mCurrentMetricItemIndex = 0;
                mCurrentMetricItemTryTimes = 0;
            }
            // if (SingletonTaskManager::Instance()->MetricItemsChanged())
            // {
            //     SingletonTaskManager::Instance()->GetMetricItems(mMetricItems, true);
            //     //更新其他统计
            //     mCurrentMetricItemIndex = 0;
            //     mCurrentMetricItemTryTimes = 0;
            // }
            if (mMetricItems.empty()) {
                //等待注册信息
                LogInfo("wait for heartbeat ready!");
                seconds = 5_s;
                continue;
            }
            std::vector<CloudMsg> cloudMsgs;
            Sync(mMsgs)
                    {
                        cloudMsgs.assign(mMsgs.begin(), mMsgs.end());
                        mMsgs.clear();
                    }}}
            if (!cloudMsgs.empty()) {
                queueEmptyCounter = 0;
                string sendBody;
                size_t count = ToPayloadMsgs(cloudMsgs, sendBody);
                SendMsg(count, sendBody);
                LogDebug("send {}/{}, result to cloudmonitor spend {} ms", cloudMsgs.size(), count,
                         timeProfile.millis(false));
            } else {
                uint32_t emptyCount = ++queueEmptyCounter;
                LogInfo("msg queue is empty {}!!", emptyCount);
            }
        }
        seconds = mInterval - timeProfile.cost<std::chrono::microseconds>(true);
        //send metric
        /*
        string body;
        CloudMetricConfig metricConfig;
        while(GetSendMetricBody(body,metricConfig))
        {
            SendMetric(body,metricConfig);
        }*/
    }
}

size_t CloudChannel::ToPayloadMsgs(const std::vector<CloudMsg> &cloudMsgs, string &body) const {
    vector<string> lines;
    lines.reserve(128);
    for (const auto &cloudMsg: cloudMsgs) {
        CollectData collectData;
        string msg = cloudMsg.msg;
        ModuleData::convertStringToCollectData(msg, collectData);
        if (collectData.moduleName != cloudMsg.name) {
            LogWarn("skip invalid moduleName {}:{}", collectData.moduleName, cloudMsg.name);
            continue;
        }
        if (collectData.dataVector.empty()) {
            LogWarn("skip empty data moduleName {}", collectData.moduleName);
            continue;
        }
        int errCount = 0;
        for (size_t d = 0; d < collectData.dataVector.size(); d++) {
            string line = ToPayloadMetricData(collectData.dataVector[d], cloudMsg.timestamp);
            if (!line.empty()) {
                lines.push_back(line);
            } else {
                errCount++;
                LogWarn("skip empty invalid MetricData[{}]: {}", d, collectData.moduleName);
            }
        }
        if (errCount > 0) {
            LogWarn("invalid MetricData: {}", msg);
        }
    }
    sort(lines.begin(), lines.end());

    size_t size = 0;
    for (const auto &line: lines) {
        size += line.size();
    }
    body.reserve(body.size() + size);
    for (const auto &line: lines) {
        body += line;
    }
    return lines.size();
}

std::string CloudChannel::ToPayloadMetricData(const MetricData &metricData,
                                              const std::chrono::system_clock::time_point &timestamp) const {
    if (!metricData.check()) {
        return {}; // 无效metric
    }

    string metricName;
    string ns;
    string instanceId;
    string userId;
    string content;
    for (const auto &tagIt: metricData.tagMap) {
        if (tagIt.first == "metricName") {
            metricName = tagIt.second;
        } else if (tagIt.first == "ns") {
            ns = tagIt.second;
        } else {
            content += " " + tagIt.first + "=" + HttpClient::UrlEncode(tagIt.second);
        }
    }
    double metricValue = 0;
    for (const auto &valueIt: metricData.valueMap) {
        if (valueIt.first == "metricValue") {
            metricValue = valueIt.second;
        } else {
            content += " " + valueIt.first + "=" + ToPayloadString(valueIt.second);
        }
    }
    argus::NodeItem curNode = this->nodeItem();
    content += " instanceId=" + HttpClient::UrlEncode(curNode.instanceId);
    content += " userId=" + curNode.aliUid;

    std::string line = metricName +
                       " " + StringUtils::NumberToString(ToMillis(timestamp)) +
                       " " + ToPayloadString(metricValue) +
                       " ns=" + UrlEncode(ns);
    line += content + "\n";

    return line;
}

bool CloudChannel::doSend(const char *type, const HttpRequest &req, HttpResponse &resp, int tryTimes) const {
    std::string targetUrl = req.url + (req.proxy.empty() ? req.proxy : fmt::format("(proxy: {})", req.proxy));

    if (fnHttpPost(req, resp) == HttpClient::Success) {
        if (resp.resCode == (int) HttpStatus::OK) {
            //还需要进一步解析resp.result
            if (!resp.result.empty() && !ParseResponseResult(resp.result)) {
                LogWarn("send {} to {} with error responseCode={}, msg={}, tryTimes={}",
                        type, targetUrl, resp.resCode, resp.result, tryTimes);
            } else {
                LogInfo("send {} to {}, success, len={}", type, targetUrl, req.body.size());
                for (size_t i = 0, step = 10 * 1000; i < req.body.size(); i += step) {
                    std::string sub = req.body.substr(i, step);
                    LogDebug("[{}] [{} B]{}", i, sub.size(), sub);
                }
                return true;
            }
        } else {
            LogWarn("send {} to {} with error responseCode={},msg={},tryTimes={}", type,
                    targetUrl, resp.resCode, resp.result, tryTimes);
            LogDebug("send metric to {} error,origin body={}", targetUrl, req.body);
        }

    } else {
        //一般为超时,建议重试
        LogWarn("send {} to {} error={}, tryTimes={}", type, targetUrl, resp.errorMsg, tryTimes);
    }

    return false;
}

bool CloudChannel::SendMsg(size_t count, const string &sendBody) {
    if (mMetricItems.empty()) {
        LogWarn("metricHubs is empty,can't send collect to metricHub");
        return true;
    }
    // bool sendOK = false;
    if (mCurrentMetricItemTryTimes >= 3) {
        mCurrentMetricItemIndex = (mCurrentMetricItemIndex + 1) % mMetricItems.size();
        mCurrentMetricItemTryTimes = 0;
    }

    HttpRequest httpRequest;
    if (mMetricItems[mCurrentMetricItemIndex].useProxy) {
        httpRequest.proxy = mCloudAgentInfo.proxyUrl;
        httpRequest.user = mCloudAgentInfo.user;
        httpRequest.password = mCloudAgentInfo.password;
    }
    httpRequest.timeout = 15;
    httpRequest.url = mMetricItems[mCurrentMetricItemIndex].url;
    if (!mMetricItems[mCurrentMetricItemIndex].gzip) {
        httpRequest.httpHeader.emplace_back("Content-Type: text/plain");
        httpRequest.body = sendBody;
    } else {
        httpRequest.httpHeader.emplace_back("Content-Encoding: gzip");
        GZip::Compress(sendBody, httpRequest.body);
    }
    if (!mCloudAgentInfo.accessSecret.empty() && !mCloudAgentInfo.accessKeyId.empty()) {
        //ak不为空，则需要计算签名
        string sign;
        CloudSignature::Calculate(httpRequest.body, mCloudAgentInfo.accessSecret, sign);
        string cmsAccessKey = "cms-access-key: " + mCloudAgentInfo.accessKeyId;
        string cmsSignature = "cms-signature: " + sign;
        httpRequest.httpHeader.push_back(cmsAccessKey);
        httpRequest.httpHeader.push_back(cmsSignature);
    }
    LogDebug("will send metric with length: {}", httpRequest.body.size());
    // Logs目录下保存最后一次上报的指标
    WriteFileContent(SingletonConfig::Instance()->getLogDir() / "argus-last-send-cms.txt",
                     fmt::format("{:L}\n{}", std::chrono::Now(), sendBody));

    int resCode = -1;
    string errorMsg;
    const double errorRate = double(mLastContinueErrorCount) / 4;
    TimeProfile timeProfile;
    // if (fnHttpPost(httpRequest, httpResponse) == HttpClient::Success)
    // {
    //     if (httpResponse.resCode == 200)
    //     {
    //         //还需要进一步解析httpResponse.result
    //         if (!httpResponse.result.empty() && !ParseResponseResult(httpResponse.result))
    //         {
    //             mErrorSendCount++;
    //             mLastContinueErrorCount++;
    //             mCurrentMetricItemTryTimes++;
    //             errorMsg = httpResponse.result;
    //             LogWarn("send metric to {} with error responseCode={},msg={}",
    //                      httpRequest.url.c_str(),
    //                      httpResponse.resCode,
    //                      httpResponse.result.c_str());
    //         }
    //         else
    //         {
    //             sendOK = true;
    //             mCurrentMetricItemTryTimes = 0;
    //             mLastContinueErrorCount = 0;
    //             mOkSendCount++;
    //             LogInfo("send metric to {} success,len={}, records={}",
    //                      httpRequest.url, httpRequest.body.size(), count);
    //         }
    //     }
    //     else
    //     {
    //         mErrorSendCount++;
    //         mLastContinueErrorCount++;
    //         mCurrentMetricItemTryTimes++;
    //         errorMsg = httpResponse.result;
    //         LogWarn("send metric to {} with error responseCode={},msg={}",
    //                  httpRequest.url.c_str(),
    //                  httpResponse.resCode,
    //                  httpResponse.result.c_str());
    //     }
    //     resCode = httpResponse.resCode;
    // }
    // else
    // {
    //     mCurrentMetricItemTryTimes++;
    //     mLastContinueErrorCount++;
    //     mErrorSendCount++;
    //     errorMsg = httpResponse.errorMsg;
    //     LogWarn("send metric to {} error={}", httpRequest.url.c_str(), httpResponse.errorMsg.c_str());
    // }
    HttpResponse httpResponse;
    bool sendOK = doSend("msg", httpRequest, httpResponse, 0);
    if (sendOK) {
        mCurrentMetricItemTryTimes = 0;
        mLastContinueErrorCount = 0;
        mOkSendCount++;
    } else {
        mErrorSendCount++;
        mLastContinueErrorCount++;
        mCurrentMetricItemTryTimes++;
    }
    mTotalSendCount++;
    LogInfo("send metric summary,total={},ok={},error={}, records={}",
            mTotalSendCount, mOkSendCount, mErrorSendCount, count);
    CollectInfo *pCollectInfo = SingletonCollectInfo::Instance();
    pCollectInfo->SetLastCommitCode(resCode);
    pCollectInfo->SetLastCommitMsg(errorMsg);
    pCollectInfo->SetLastCommitCost(static_cast<double>(timeProfile.millis()));
    //pCollectInfo->SetLastCommitTime(endTime);
    pCollectInfo->SetPutMetricFailCount(mErrorSendCount);
    pCollectInfo->SetPutMetricSuccCount(mOkSendCount);
    pCollectInfo->SetPutMetricFailPerMinute(errorRate);

    return sendOK;
}

bool CloudChannel::ParseResponseResult(const string &result) {
    std::string error;
    json::Object jsonValue = json::parseObject(result, error);
    if (jsonValue.isNull()) {
        LogWarn("the response result({}) is invalid: {}", result, error);
        return false;
    }

    string code = jsonValue.getString("code");
    bool succeed = StringUtils::EqualIgnoreCase(code, "Success");
    Log((succeed ? LEVEL_DEBUG : LEVEL_WARN), "the response code is {}", code);
    return succeed;
}

bool CloudChannel::ParseCloudMetricConfig(const string &conf, CloudMetricConfig &config) {
    std::string error;
    json::Object jsonValue = json::parseObject(conf, error);
    if (jsonValue.isNull()) {
        LogWarn("the conf is not valid: {}, conf: {}", error, conf);
        return false;
    }

    // CommonJson::GetJsonStringValue(value, "acckeyId", config.acckeyId);
    config.acckeyId = jsonValue.getString("acckeyId");
    // CommonJson::GetJsonStringValue(value, "accKeySecretSign", config.accKeySecretSign);
    config.accKeySecretSign = jsonValue.getString("accKeySecretSign");
    // CommonJson::GetJsonStringValue(value, "uploadEndpoint", config.uploadEndpoint);
    config.uploadEndpoint = jsonValue.getString("uploadEndpoint");
    // CommonJson::GetJsonStringValue(value, "secureToken", config.secureToken);
    config.secureToken = jsonValue.getString("secureToken");
    // CommonJson::GetJsonBoolValue(value, "needTimestamp", config.needTimestamp);
    config.needTimestamp = jsonValue.getBool("needTimestamp");

    if (config.acckeyId.empty() || config.accKeySecretSign.empty() || config.uploadEndpoint.empty()) {
        LogWarn("the conf({}) is invalid!", conf);
        return false;
    }
    return true;
}

int CloudChannel::SendMetric(const string &body, const CloudMetricConfig &metricConfig) const {
    string gzipBody;
    GZip::Compress(body, gzipBody);
    std::map<std::string, std::string> header;
    GetSendMetricHeader(gzipBody, metricConfig, header);

    for (int tryTimes = 0; tryTimes < 2; tryTimes++) {
        HttpRequest httpRequest;
        httpRequest.url = metricConfig.uploadEndpoint;
        httpRequest.body = gzipBody;
        httpRequest.timeout = 2;
        for (auto &it: header) {
            httpRequest.httpHeader.push_back(it.first + ": " + it.second);
        }

        HttpResponse httpResponse;
        if (doSend("metric", httpRequest, httpResponse, tryTimes)) {
            return 0;
        }
        // if (fnHttpPost(httpRequest, httpResponse) == HttpClient::Success)
        // {
        //     if (httpResponse.resCode == (int) HttpStatus::OK)
        //     {
        //         //还需要进一步解析httpResponse.result
        //         if (!httpResponse.result.empty() && !ParseResponseResult(httpResponse.result))
        //         {
        //             tryTimes++;
        //             LogWarn("send metric to {} with error responseCode={},msg={},tryTimes={}",
        //                      httpRequest.url, httpResponse.resCode, httpResponse.result, tryTimes);
        //
        //         }
        //         else
        //         {
        //             LogInfo("send metric to {} success,len={}",
        //                      httpRequest.url.c_str(),
        //                      httpRequest.body.size());
        //             LogDebug("send metric to {} success,origin body={}",
        //                      httpRequest.url.c_str(),
        //                      body.c_str());
        //             return 0;
        //         }
        //     }
        //     else
        //     {
        //         tryTimes++;
        //         LogWarn("send metric to {} with error responseCode={},msg={},tryTimes={}",
        //                  httpRequest.url.c_str(),
        //                  httpResponse.resCode,
        //                  httpResponse.result.c_str(),
        //                  tryTimes);
        //         LogDebug("send metric to {} error,origin body={}",
        //                      httpRequest.url.c_str(),
        //                      body.c_str());
        //     }
        //
        // }
        // else
        // {
        //     //一般为超时,建议重试
        //     tryTimes++;
        //     LogWarn("send metric to {} error={},tryTimes={}",
        //              httpRequest.url,
        //              httpResponse.errorMsg,
        //              tryTimes);
        // }
#ifndef ENABLE_COVERAGE
        std::this_thread::sleep_for(std::chrono::seconds{2});
#endif
    }
    return -1;
}

int CloudChannel::GetSendMetricHeader(const string &body,
                                      const CloudMetricConfig &metricConfig,
                                      map<string, string> &header) const {
    header.clear();
    header["User-Agent"] = "Argus";
    header["Content-MD5"] = MD5String(body.c_str(), body.size());
    header["Content-Type"] = "text/plain";
    header["Content-Encoding"] = "gzip";
    // FIX: https://aone.alibaba-inc.com/v2/project/2004651/bug/46379451# 《云监控agent 特定时区上报数据失败》
    header["Date"] = fmt::format("{:%a, %d %b %Y %H:%M:%S} GMT", std::chrono::system_clock::now());
    header["x-cms-api-version"] = "1.1";
    header["x-cms-agent-version"] = getVersion(false);
    header["x-cms-agent-instance"] = mNodeItem.instanceId;
    header["x-cms-host"] = mHostname;
    header["x-cms-ip"] = mMainIp;
    header["x-cms-signature"] = "hmac-sha1";
    if (!metricConfig.secureToken.empty()) {
        header["x-cms-security-token"] = metricConfig.secureToken;
        header["x-cms-caller-type"] = "token";
    }

    // SRS: https://aone.alibaba-inc.com/v2/project/899492/req/47560201# 《agent上报的数据信息里，增加自己身份相关的header》
    header["x-cms-instance-sn"] = mNodeItem.serialNumber;

    // 签名
    string signString = "POST\n";
    signString += header["Content-MD5"] + "\n";
    signString += header["Content-Type"] + "\n";
    signString += header["Date"] + "\n";
    for (auto &it: header) {
        if (it.first.find("x-cms") != string::npos) {
            signString += it.first + ":" + it.second + "\n";
        }
    }
    string host, path;
    HttpClient::UrlParse(metricConfig.uploadEndpoint, host, path);
    signString += path;
    string sign;
    CloudSignature::CalculateMetric(signString, metricConfig.accKeySecretSign, sign);
    LogDebug("accKeySecretSign={},sign={}, signString={}", metricConfig.accKeySecretSign, sign, signString);
    header["Authorization"] = metricConfig.acckeyId + ":" + sign;

    return 0;
}
