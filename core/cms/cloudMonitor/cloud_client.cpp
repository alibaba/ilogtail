#include "cloud_client.h"

#include <memory>
#include <boost/json.hpp>

#include "proxy_manager.h"
#include "collect_info.h"
#include "system_info.h"
#include "cloud_signature.h"

#include "common/HttpClient.h"
#include "common/Logger.h"
#include "common/JsonValueEx.h"
#include "common/StringUtils.h"
#include "common/Md5.h"
#include "common/Base64.h"
#include "common/ExceptionsHandler.h"
#include "common/SystemUtil.h"
#include "common/ResourceConsumptionRecord.h"
#include "common/FilePathUtils.h"
#include "common/Chrono.h"
#include "common/CpuArch.h"

#include "common/ThreadUtils.h"

#include "core/TaskManager.h"
#include "core/task_monitor.h"
#include "common/Config.h"

using namespace std;
using namespace argus;
using namespace common;

namespace cloudMonitor {
    void ParseNodeInfo(json::Object &);
    void ParseMetricHubInfo(json::Object &);
    bool IsEqual(const std::vector<MetricItem> &, const std::vector<MetricItem> &);

    CloudClient::CloudClient() {
        mCloudAgentInfo = std::make_shared<CloudAgentInfo>();
    }

    CloudClient::~CloudClient() {
        LogInfo("the CloudClient will exit!");
        stopAsync();
    }

    std::string makeDumpBodyJson(const std::string &type,
                                 const boost::json::value &detail,
                                 const argus::CloudAgentInfo &agentInfo) {
        using namespace boost::json;

        NodeItem nodeItem;
        SingletonTaskManager::Instance()->GetNodeItem(nodeItem);

        // See: https://aone.alibaba-inc.com/v2/project/897205/req/50590640
        // 《argusagent上报minidump时,cloudmonitor不必进行合法性检查》
        value dump = {
                {"version",  getVersion(false)},
                {"type",     type},
                {"hostname", SystemUtil::getHostname()},
                {"detail",   detail},
        };
        value jv;
        if (nodeItem.instanceId.empty()) {
            dump.if_object()->emplace("sn", agentInfo.serialNumber);
            jv = value{
                    {"sn",           agentInfo.serialNumber},
                    {"agentVersion", getVersion(false)},
                    {"targetOsArch", OS_NAME "-" CPU_ARCH},
                    {"dump",         serialize(dump)},
            };
        } else {
            dump.if_object()->emplace("sn", nodeItem.serialNumber);
            jv = value{
                    {"__ver__",      "2.0"},
                    {"sn",           nodeItem.serialNumber},
                    {"agentVersion", getVersion(false)},
                    {"userId",       nodeItem.aliUid},
                    {"instanceId",   nodeItem.instanceId},
                    {"hostname",     SystemUtil::getHostname()},
                    {"os",           nodeItem.operatingSystem},
                    {"targetOsArch", OS_NAME "-" CPU_ARCH},
                    {"region",       nodeItem.region},
                    {"type",         type},
                    {"dump",         serialize(dump)},
            };
        }

        return serialize(jv);
    }

    std::string makeCoreDumpJsonBody(const tagCoreDumpFile &dumpFile, const argus::CloudAgentInfo &agentInfo) {
        return makeDumpBodyJson("CoreDown", boost::json::value_from(dumpFile.toJson()), agentInfo);
    }

    // See: https://aone.alibaba-inc.com/v2/project/805897/req/49773817# 《minidump上报》
    bool CloudClient::SaveDump(const std::string &type, const std::string &body) const {
        HttpRequest req;
        CompleteHttpRequest(req, "/agent/saveMiniDump", body, "text/json");

        HttpResponse resp;
        LogDebug("will send {}({}): {}", type, req.url, req.body);
        bool ok = HttpClient::HttpPost(req, resp) == HttpClient::Success
                  && resp.resCode == (int) HttpStatus::OK
                  && json::parseObject(resp.result, resp.errorMsg).getBool("success", false);
        Log((ok ? LEVEL_INFO : LEVEL_ERROR),
            "send {} to {} {}, len={}, http status code: {} response: {}{}",
            type, req.url, (ok ? "success" : "fail"), req.body.size(), resp.resCode, resp.result,
            (resp.errorMsg.empty() ? "" : ", errMsg: " + resp.errorMsg));

        return ok;
    }

    bool CloudClient::SendCoreDump() const {
        bool ok = false;
        DumpCache dump(1, true);
        auto lastCoreDown = dump.GetLastCoreDown(std::chrono::seconds{60 * 60});
        if (!lastCoreDown.filename.empty()) {
            std::string body = makeCoreDumpJsonBody(lastCoreDown, *mCloudAgentInfo);
            ok = this->SaveDump("CoreDown", body);
        }

        return ok;
    }

    std::string makeThreadDumpJsonBody(const std::vector<ResourceWaterLevel> &vecResources,
                                       const std::vector<ResourceConsumption> &topTasks,
#if !defined(WITHOUT_MINI_DUMP)
                                       const std::list<tagThreadStack> &lstThreadStack,
#endif
                                       const argus::CloudAgentInfo &agentInfo) {
        boost::json::array resources;
        if (!vecResources.empty()) {
            resources.resize(vecResources.size());
            int index = 0;
            for (const auto &res: vecResources) {
                resources[index++] = boost::json::value{
                        {"resource",  res.name},
                        {"value",     res.value},
                        {"threshold", res.threshold},
                        {"times",     res.times},
                };
            }
        }

        boost::json::array topN;
        if (!topTasks.empty()) {
            topN.resize(topTasks.size());
            int index = 0;
            for (const auto &task: topTasks) {
                topN[index++] = boost::json::value{
                        {"threadId", task.ThreadId},
                        {"millis",   ToMillis(task.Millis)}, // 耗时
                        {"taskName", task.TaskName},
                };
            }
        }

#if !defined(WITHOUT_MINI_DUMP)
        boost::json::value threadDumps;
        if (!lstThreadStack.empty()) {
            std::stringstream ss;
            int index = 0;
            for (const auto &thread: lstThreadStack) {
                ss << thread.str(index++) << std::endl;
            }
            threadDumps = boost::json::value{ss.str()};
        }
#endif // !WITHOUT_MINI_DUMP

        return makeDumpBodyJson("ThreadDump", boost::json::value{
                {"resources", resources},
                {"topTasks",  topN},
#if !defined(WITHOUT_MINI_DUMP)
                {"threads",   threadDumps},
#endif
        }, agentInfo);
    }

    bool CloudClient::SendThreadsDump(const std::vector<ResourceWaterLevel> &resources,
                                      const std::vector<ResourceConsumption> &topTasks
#if !defined(WITHOUT_MINI_DUMP)
                                      , const std::list<tagThreadStack> &lstStack
#endif
                                      ) const {
        std::string body = makeThreadDumpJsonBody(resources, topTasks,
#if !defined(WITHOUT_MINI_DUMP)
            lstStack,
#endif
            *mCloudAgentInfo);
        return this->SaveDump("ThreadDump", body);
    }

    void CloudClient::Start() {
        InitPluginLib();
        InitProxyManager();

        auto backgroundTask = [this]() {
            std::chrono::seconds next = mInterval;
            string response;
            if (SendHeartbeat(response)) {
                DealHeartBeatResponse(response);
            } else if (mContinueErrorCount > 3) {
                mContinueErrorCount = 0;
                //持续错误，需要重新判断网络环境
                InitProxyManager();
                next = std::chrono::seconds{5};
            } else if (mOkCount == 0) {
                //没有成功过，需要加快重试频率
                next = mFirstInterval;
            }
            //其他错误
            return next;
        };
        auto delay = backgroundTask();
#ifndef ENABLE_COVERAGE
        // 测试情况下不主动发送core dump
        SendCoreDump();
#endif

        this->runAsync(backgroundTask, delay);
    }

    void CloudClient::InitProxyManager() const {
        ProxyManager pManager;
        pManager.Init();
        SingletonTaskManager::Instance()->GetCloudAgentInfo(*mCloudAgentInfo);
    }

    void CloudClient::CompleteHttpRequest(HttpRequest &request, const std::string &uri, const std::string &body,
                                          const std::string &contentType) const {
        request.proxy = mCloudAgentInfo->proxyUrl;
        request.user = mCloudAgentInfo->user;
        request.password = mCloudAgentInfo->password;
        request.timeout = 15;
        request.url = mCloudAgentInfo->HeartbeatUrl + uri;//"/agent/heartbeat";
        request.httpHeader.push_back("Content-Type: " + contentType);// .emplace_back("Content-Type: text/plain");
        request.body = body;
        if (!mCloudAgentInfo->accessSecret.empty() && !mCloudAgentInfo->accessKeyId.empty()) {
            //ak不为空，则需要计算签名
            string sign;
            CloudSignature::Calculate(request.body, mCloudAgentInfo->accessSecret, sign);
            string cmsAccessKey = "cms-access-key: " + mCloudAgentInfo->accessKeyId;
            string cmsSignature = "cms-signature: " + sign;
            request.httpHeader.push_back(cmsAccessKey);
            request.httpHeader.push_back(cmsSignature);
        }
    }

    bool CloudClient::SendHeartbeat(const std::string &req, string &respBody) {
        HttpRequest request;
        CompleteHttpRequest(request, "/agent/heartbeat", req);
        HttpResponse response;
        double errorRate = 0.0;
        bool bSuccess = false;

        LogInfo("send heartbeat [POST]{}, proxy: <{}>, body: {}", request.url, request.proxy, request.body);
        if (HttpClient::HttpPost(request, response) == HttpClient::Success) {
            LogDebug("http response result--responseCode={},responseResult={}", response.resCode, response.result);
            if (response.resCode == (int) HttpStatus::OK) {
                bSuccess = true;
                respBody = response.result;
                LogInfo("send heartbeat to [POST]{} success, len={}", request.url, response.result.size());
            }
        }
        if (bSuccess) {
            mOkCount++;
            mContinueErrorCount = 0;
        }else {
            mErrorCount++;
            mContinueErrorCount++;
            errorRate = 1.0 / 3;
            LogWarn("send heartbeat to [POST]{} with error HttpStatusCode={}, errMsg={}, body={}",
                    request.url, response.resCode, response.errorMsg, response.result);
        }
        SingletonCollectInfo::Instance()->SetPullConfigFailCount(mErrorCount);
        SingletonCollectInfo::Instance()->SetPullConfigFailPerMinute(errorRate);
        SingletonCollectInfo::Instance()->SetPullConfigSuccCount(mOkCount);
        return bSuccess;
    }

    bool CloudClient::SendHeartbeat(string &respBody) {
        string sendBody;
        GetHeartbeatJsonString(sendBody);
        return SendHeartbeat(sendBody, respBody);
    }

    // 样例: {"systemInfo":{"serialNumber":"61fddc94-3544-11eb-9a78-e3b6c8469a4b","hostname":"dev","localIPs":["11.160.139.211"],"name":"Linux (Red Hat)","version":"7.2","arch":"x86_64","freeSpace":19314408},"versionInfo":{"version": "3.5.9.11"}}
    void CloudClient::GetHeartbeatJsonString(string &json) const {
        // SystemInfo sysInfo;
        // auto systemInfo = SystemInfo{}.GetSystemInfo(mCloudAgentInfo->serialNumber);
        boost::json::object jv;
        jv["systemInfo"] = SystemInfo{}.GetSystemInfo(mCloudAgentInfo->serialNumber);
        jv["versionInfo"] = boost::json::object{
                {"version", getVersion(false)},
        };

        HpcClusterItem hpcClusterItem;
        SingletonTaskManager::Instance()->GetHpcClusterItem(hpcClusterItem);
        if (!hpcClusterItem.version.empty()) {
            jv["hpcClusterConfigVersion"] = hpcClusterItem.version;
        }

        json = boost::json::serialize(jv);
    }

    void CloudClient::DealHeartBeatResponse(const string &response) {
        LogInfo("the heartbeat response is :{}", response.c_str());
        string md5 = MD5String(response.c_str(), response.size());
        if (mResponseMd5 == md5) {
            LogDebug("the heartbeat response is the same with the last,skip parse");
            return;
        }
        mResponseMd5 = md5;

        std::string error;
        json::Object jsonObject = json::parseObject(response, error);
        if (jsonObject.isNull()) {
            LogWarn("the heartbeat response is invalid: {}, response: {}", error, response);
            return;
        }
        // Json::Value value = jsonValue.getValue();
        // if (!value.isObject()) {
        //     LogWarn("the heartbeat response is not object json");
        //     return;
        // }
        ParseNodeInfo(jsonObject);
        TaskMonitor::ParseProcessInfo(jsonObject);
        ParseMetricHubInfo(jsonObject);
        TaskMonitor::ParseHttpInfo(jsonObject);
        TaskMonitor::ParseTelnetInfo(jsonObject);
        TaskMonitor::ParsePingInfo(jsonObject);
        ParseHpcClusterConfig(jsonObject);
        ParseFileStoreInfo(jsonObject);
        SingletonTaskMonitor::Instance()->ParseUnifiedConfig(response);
    }

    void ParseNodeInfo(json::Object &jsonObject) {
        auto node = jsonObject.getObject("node");
        // string key = "node";
        // Json::Value value = jsonValue.get(key, Json::Value::null);
        // if (value.isNull() || !value.isObject()) {
        //     LogWarn("no node in the response json!");
        //     return;
        // }
        if (node.isNull()) {
            LogWarn("no node in the response json!");
            return;
        }

        NodeItem nodeItem;
        // CommonJson::GetJsonStringValue(value, "instanceId", nodeItem.instanceId);
        nodeItem.instanceId = node.getString("instanceId");
        // CommonJson::GetJsonStringValue(value, "serialNumber", nodeItem.serialNumber);
        nodeItem.serialNumber = node.getString("serialNumber");

        // double aliUid = 0;
        // CommonJson::GetJsonDoubleValue(value, "aliUid", aliUid);
        nodeItem.aliUid = convert(node.getNumber<uint64_t>("aliUid"));
        // CommonJson::GetJsonStringValue(value, "hostName", nodeItem.hostName);
        nodeItem.hostName = node.getString("hostName");
        // CommonJson::GetJsonStringValue(value, "operatingSystem", nodeItem.operatingSystem);
        nodeItem.operatingSystem = node.getString("operatingSystem");
        // CommonJson::GetJsonStringValue(value, "region", nodeItem.region);
        nodeItem.region = node.getString("region");
        LogInfo(R"(dimension: {{"userId":"{}","instanceId":"{}"}})", nodeItem.aliUid, nodeItem.instanceId);
        SingletonTaskManager::Instance()->SetNodeItem(nodeItem);
    }

    void ParseMetricHubInfo(json::Object &jsonValue) {
        auto metricItems = std::make_shared<vector<MetricItem>>();
        string metricHubUrl = SingletonConfig::Instance()->GetValue("cms.metrichub_url", "");

        auto extractMetricItem = [metricHubUrl](json::Object &value, bool withMetricHubUrl) {
            MetricItem metricItem;
            // CommonJson::GetJsonStringValue(value, "url", metricItem.url);
            if (withMetricHubUrl && !metricHubUrl.empty()) {
                metricItem.url = metricHubUrl;
            } else {
                metricItem.url = value.getString("url");
            }
            // CommonJson::GetJsonBoolValue(value, "gzip", metricItem.gzip);
            metricItem.gzip = value.getBool("gzip");
            // CommonJson::GetJsonBoolValue(value, "useProxy", metricItem.useProxy);
            metricItem.useProxy = value.getBool("useProxy");

            return metricItem;
        };
        json::Object value = jsonValue.getObject("metricHubConfig");
        if (value.isNull()) {
            LogWarn("no metricHubConfig in the response json!");
        } else {
            metricItems->push_back(extractMetricItem(value, true));
        }

        value = jsonValue.getObject("metricConfig");
        if (value.isNull()) {
            LogWarn("no metricConfig in the response json!");
        } else {
            metricItems->push_back(extractMetricItem(value, false));
        }

        // string key = "metricHubConfig";
        // Json::Value value = jsonValue.get(key, Json::Value::null);
        // if (value.isNull() || !value.isObject()) {
        //     LogWarn("no metricHubConfig in the response json!");
        // } else {
        //     MetricItem metricItem;
        //     CommonJson::GetJsonStringValue(value, "url", metricItem.url);
        //     if (!metricHubUrl.empty()) {
        //         metricItem.url = metricHubUrl;
        //     }
        //     CommonJson::GetJsonBoolValue(value, "gzip", metricItem.gzip);
        //     CommonJson::GetJsonBoolValue(value, "useProxy", metricItem.useProxy);
        //     metricItems->push_back(metricItem);
        // }
        // key = "metricConfig";
        // value = jsonValue.get(key, Json::Value::null);
        // if (value.isNull() || !value.isObject()) {
        //     LogWarn("no metricConfig in the response json!");
        // } else {
        //     MetricItem metricItem;
        //     CommonJson::GetJsonStringValue(value, "url", metricItem.url);
        //     CommonJson::GetJsonBoolValue(value, "gzip", metricItem.gzip);
        //     CommonJson::GetJsonBoolValue(value, "useProxy", metricItem.useProxy);
        //     metricItems->push_back(metricItem);
        // }
        auto currentMetricItems = SingletonTaskManager::Instance()->MetricItems().Get();
        if (!IsEqual(*currentMetricItems, *metricItems)) {
            LogInfo("metricConfig is not the same,will use new metricConfig");
            SingletonTaskManager::Instance()->SetMetricItems(metricItems);
        } else {
            LogInfo("metricConfig is the same,no change!");
        }
    }

    void CloudClient::ParseHpcClusterConfig(const json::Object &jsonValue) {
        HpcClusterItem hpcClusterItem;
        json::Object value = jsonValue.getObject("hpcClusterConfig");
        if (value.isNull()) {
            SingletonTaskManager::Instance()->GetHpcClusterItem(hpcClusterItem);
            LogInfo("no hpcClusterConfig in the response json, {}",
                    (hpcClusterItem.isValid ? "use cached" : "no rdma"));
            return;
        }

        value.get("clusterId", hpcClusterItem.clusterId);
        value.get("regionId", hpcClusterItem.regionId);
        value.get("version", hpcClusterItem.version);
        if (!hpcClusterItem.version.empty()) {
            hpcClusterItem.isValid = true;
        }

        value.getArray("instances").forEach([&](size_t, const json::Object &instanceItem) {
            HpcNodeInstance hpcNodeInstance;
            instanceItem.get("instanceId", hpcNodeInstance.instanceId);
            instanceItem.get("ip", hpcNodeInstance.ip);
            hpcClusterItem.hpcNodeInstances.push_back(hpcNodeInstance);
        });

        SingletonTaskManager::Instance()->SetHpcClusterItem(hpcClusterItem);
    }

    void CloudClient::ParseFileStoreInfo(const json::Object &jsonValue) {
        json::Array value = jsonValue.getArray("fileStore");
        if (value.isNull()) {
            LogWarn("no fileStore in the response json");
            return;
        }
        value.forEach([&](size_t, const json::Object &item) {
            string filePath = item.getString("filePath");
            string content = item.getString("content");
            string user = item.getString("user", "root");
            if (filePath.empty() || content.empty() || user.empty()) {
                LogWarn("fileStore file path or content or user empty, skip!");
            }
            if (!StoreFile(filePath, content, user)) {
                LogWarn("store file: {} failed!", filePath);
            }
        });
    }

    bool CloudClient::StoreFile(const fs::path &filePath, const std::string &content, const std::string &user) {
        if (filePath.empty()) {
            LogWarn("file absolute path resolved is empty, skip file: {}", filePath.string());
            return false;
        }
        fs::path absFilePath{filePath};
        if (absFilePath.is_relative()) {
            absFilePath = SingletonConfig::Instance()->getBaseDir() / filePath;
        }

        string fileContent = Base64::decode(content);
        if (fileContent.empty()) {
            LogWarn("file content after base64 decode is empty, skip file: {}", absFilePath.string());
            return false;
        }
        string errMsg;
        if (common::StoreFile(fileContent, absFilePath.string(), user, errMsg) != 0) {
            LogError("store file error: {}, file: {}", errMsg, absFilePath.string());
            return false;
        }
        LogInfo("store file success, config file path: {}, store file path: {}",
                filePath.string(), absFilePath.string());
        return true;
    }

    void CloudClient::InitPluginLib() {
    }

    bool IsEqual(const std::vector<MetricItem> &l, const std::vector<MetricItem> &r) {
        bool equal = l.size() == r.size();
        for (size_t i = 0; i < l.size() && equal; i++) {
            equal = l[i].gzip == r[i].gzip &&
                    l[i].url == r[i].url &&
                    l[i].useProxy == r[i].useProxy;
        }
        return equal;
    }
} // namespace cloudMonitor
