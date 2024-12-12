// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Client.h"

#include "Common.h"
#include "CurlImp.h"
#include "Exception.h"
#include "Result.h"
#include "app_config/AppConfig.h"
#include "common/Flags.h"
#include "logger/Logger.h"
#include "monitor/Monitor.h"
#include "plugin/flusher/sls/SLSClientManager.h"
#ifdef __ENTERPRISE__
#include "plugin/flusher/sls/EnterpriseSLSClientManager.h"
#endif

DECLARE_FLAG_STRING(default_aliuid);

namespace logtail {
namespace sdk {

    using namespace std;

    Client::Client(const string& aliuid, const string& slsHost, int32_t timeout)
        : mTimeout(timeout), mHostFieldSuffix(""), mIsHostRawIp(false), mPort(80), mUsingHTTPS(false), mAliuid(aliuid) {
        mClient = new CurlClient();
        mSlsHostUpdateTime = 0;
        mSlsRealIpUpdateTime = 0;
        SetSlsHost(slsHost);
        if (mTimeout <= 0) {
            mTimeout = LOG_REQUEST_TIMEOUT;
        }
    }

    Client::~Client() throw() {
        if (mClient != NULL) {
            delete mClient;
        }
    }

    void Client::SetPort(int32_t port) {
        mPort = port;
        mUsingHTTPS = (443 == mPort);
    }

    string Client::GetSlsHost() {
        mSpinLock.lock();
        string slsHost = mSlsHost;
        mSpinLock.unlock();
        return slsHost;
    }

    string Client::GetRawSlsHost() {
        mSpinLock.lock();
        string rawSlsHost = mRawSlsHost;
        mSpinLock.unlock();
        return rawSlsHost;
    }

    string Client::GetHostFieldSuffix() {
        mSpinLock.lock();
        string hostFieldSuffix = mHostFieldSuffix;
        mSpinLock.unlock();
        return hostFieldSuffix;
    }

    bool Client::GetRawSlsHostFlag() {
        return mIsHostRawIp;
    }

    void Client::SetSlsHost(const string& slsHost) {
        mSpinLock.lock();
        if (slsHost == mRawSlsHost) {
            mSpinLock.unlock();
            return;
        }
        mRawSlsHost = slsHost;
        size_t bpos = slsHost.find("://");
        if (bpos == string::npos)
            bpos = 0;
        else
            bpos += 3;
        string tmpstr = slsHost.substr(bpos);
        size_t epos = tmpstr.find_first_of("/");
        if (epos == string::npos)
            epos = tmpstr.length();
        string host = tmpstr.substr(0, epos);

        mSlsHost = host;

        mHostFieldSuffix = "." + host;
        size_t i = 0;
        for (; i < host.length(); ++i) {
            if ((host[i] >= 'a' && host[i] <= 'z') || (host[i] >= 'A' && host[i] <= 'Z'))
                break;
        }
        if (i == host.length())
            mIsHostRawIp = true;
        else
            mIsHostRawIp = false;
        mSpinLock.unlock();
    }


    string Client::GetHost(const string& project) {
        if (mIsHostRawIp || project.empty()) {
            return GetSlsHost();
        } else {
            return project + GetHostFieldSuffix();
        }
    }

    GetRealIpResponse Client::GetRealIp() {
        static string project = "logtail-real-ip-project";
        static string logstore = "logtail-real-ip-logstore";
        GetRealIpResponse rsp;
        try {
            PingSLSServer(project, logstore, &rsp.realIp);
        } catch (const LOGException&) {
        }
        return rsp;
    }

    bool Client::TestNetwork() {
        static string project = "logtail-test-network-project";
        static string logstore = "logtail-test-network-logstore";
        PingSLSServer(project, logstore);
        return true;
    }

    PostLogStoreLogsResponse Client::PostLogStoreLogs(const std::string& project,
                                                      const std::string& logstore,
                                                      sls_logs::SlsCompressType compressType,
                                                      const std::string& compressedLogGroup,
                                                      uint32_t rawSize,
                                                      const std::string& hashKey,
                                                      bool isTimeSeries) {
        map<string, string> httpHeader;
        httpHeader[CONTENT_TYPE] = TYPE_LOG_PROTOBUF;
        httpHeader[X_LOG_BODYRAWSIZE] = std::to_string(rawSize);
        httpHeader[X_LOG_COMPRESSTYPE] = Client::GetCompressTypeString(compressType);
        if (isTimeSeries) {
            return SynPostMetricStoreLogs(project, logstore, compressedLogGroup, httpHeader);
        } else {
            return SynPostLogStoreLogs(project, logstore, compressedLogGroup, httpHeader, hashKey);
        }
    }

    PostLogStoreLogsResponse Client::PostLogStoreLogPackageList(const std::string& project,
                                                                const std::string& logstore,
                                                                sls_logs::SlsCompressType compressType,
                                                                const std::string& packageListData,
                                                                const std::string& hashKey) {
        map<string, string> httpHeader;
        httpHeader[CONTENT_TYPE] = TYPE_LOG_PROTOBUF;
        httpHeader[X_LOG_MODE] = LOG_MODE_BATCH_GROUP;
        httpHeader[X_LOG_BODYRAWSIZE] = std::to_string(packageListData.size());
        httpHeader[X_LOG_COMPRESSTYPE] = Client::GetCompressTypeString(compressType);
        return SynPostLogStoreLogs(project, logstore, packageListData, httpHeader, hashKey);
    }

    unique_ptr<HttpSinkRequest> Client::CreatePostLogStoreLogsRequest(const std::string& project,
                                                                      const std::string& logstore,
                                                                      sls_logs::SlsCompressType compressType,
                                                                      const std::string& compressedLogGroup,
                                                                      uint32_t rawSize,
                                                                      SenderQueueItem* item,
                                                                      const std::string& hashKey,
                                                                      int64_t hashKeySeqID,
                                                                      bool isTimeSeries) {
        map<string, string> httpHeader;
        httpHeader[CONTENT_TYPE] = TYPE_LOG_PROTOBUF;
        httpHeader[X_LOG_BODYRAWSIZE] = std::to_string(rawSize);
        httpHeader[X_LOG_COMPRESSTYPE] = Client::GetCompressTypeString(compressType);
        if (isTimeSeries) {
            return CreateAsynPostMetricStoreLogsRequest(project, logstore, compressedLogGroup, httpHeader, item);
        } else {
            return CreateAsynPostLogStoreLogsRequest(
                project, logstore, compressedLogGroup, httpHeader, hashKey, hashKeySeqID, item);
        }
    }


    unique_ptr<HttpSinkRequest> Client::CreatePostLogStoreLogPackageListRequest(const std::string& project,
                                                                                const std::string& logstore,
                                                                                sls_logs::SlsCompressType compressType,
                                                                                const std::string& packageListData,
                                                                                SenderQueueItem* item,
                                                                                const std::string& hashKey) {
        map<string, string> httpHeader;
        httpHeader[CONTENT_TYPE] = TYPE_LOG_PROTOBUF;
        httpHeader[X_LOG_MODE] = LOG_MODE_BATCH_GROUP;
        httpHeader[X_LOG_BODYRAWSIZE] = std::to_string(packageListData.size());
        httpHeader[X_LOG_COMPRESSTYPE] = Client::GetCompressTypeString(compressType);
        return CreateAsynPostLogStoreLogsRequest(
            project, logstore, packageListData, httpHeader, hashKey, kInvalidHashKeySeqID, item);
    }

    void Client::SendRequest(const std::string& project,
                             const std::string& httpMethod,
                             const std::string& url,
                             const std::string& body,
                             const std::map<std::string, std::string>& parameterList,
                             std::map<std::string, std::string>& header,
                             HttpMessage& httpMessage,
                             std::string* realIpPtr) {
        SLSClientManager::AuthType type;
        string accessKeyId, accessKeySecret;
        if (!SLSClientManager::GetInstance()->GetAccessKey(mAliuid, type, accessKeyId, accessKeySecret)) {
#ifdef __ENTERPRISE__
            static auto manager = static_cast<EnterpriseSLSClientManager*>(SLSClientManager::GetInstance());
            auto status = manager->GetProjectAnonymousWriteStatus(project);
            if (status == EnterpriseSLSClientManager::ProjectAnonymousWriteStatus::INVALID
                || status == EnterpriseSLSClientManager::ProjectAnonymousWriteStatus::PENDING) {
                return throw LOGException(LOGE_UNAUTHORIZED, "");
                ;
            }
            SLSClientManager::GetInstance()->GetAccessKey(
                STRING_FLAG(default_aliuid), type, accessKeyId, accessKeySecret);
#endif
        }
        if (type == SLSClientManager::AuthType::ANONYMOUS) {
            header[X_LOG_KEYPROVIDER] = MD5_SHA1_SALT_KEYPROVIDER;
        }

        string host = GetHost(project);
        SetCommonHeader(header, (int32_t)(body.length()), project);
        string signature = GetUrlSignature(httpMethod, url, header, parameterList, body, accessKeySecret);
        header[AUTHORIZATION] = LOG_HEADSIGNATURE_PREFIX + accessKeyId + ':' + signature;

        string queryString;
        GetQueryString(parameterList, queryString);

        int32_t port = mPort;
        if (mPort == 80 && mUsingHTTPS) {
            port = 443;
        }
        mClient->Send(httpMethod,
                      host,
                      port,
                      url,
                      queryString,
                      header,
                      body,
                      mTimeout,
                      httpMessage,
                      AppConfig::GetInstance()->GetBindInterface(),
                      mUsingHTTPS);

        if (httpMessage.statusCode != 200) {
            if (realIpPtr != NULL) {
                *realIpPtr = httpMessage.header[X_LOG_HOSTIP];
            }
            ErrorCheck(httpMessage.content, httpMessage.header[X_LOG_REQUEST_ID], httpMessage.statusCode);
        }
    }

    std::unique_ptr<HttpSinkRequest>
    Client::CreateAsynPostMetricStoreLogsRequest(const std::string& project,
                                                 const std::string& logstore,
                                                 const std::string& body,
                                                 std::map<std::string, std::string>& httpHeader,
                                                 SenderQueueItem* item) {
        SLSClientManager::AuthType type;
        string accessKeyId, accessKeySecret;
        if (!SLSClientManager::GetInstance()->GetAccessKey(mAliuid, type, accessKeyId, accessKeySecret)) {
#ifdef __ENTERPRISE__
            static auto manager = static_cast<EnterpriseSLSClientManager*>(SLSClientManager::GetInstance());
            auto status = manager->GetProjectAnonymousWriteStatus(project);
            if (status == EnterpriseSLSClientManager::ProjectAnonymousWriteStatus::INVALID
                || status == EnterpriseSLSClientManager::ProjectAnonymousWriteStatus::PENDING) {
                return nullptr;
            }
            SLSClientManager::GetInstance()->GetAccessKey(
                STRING_FLAG(default_aliuid), type, accessKeyId, accessKeySecret);
#endif
        }
        if (type == SLSClientManager::AuthType::ANONYMOUS) {
            httpHeader[X_LOG_KEYPROVIDER] = MD5_SHA1_SALT_KEYPROVIDER;
        }

        string operation = METRICSTORES;
        operation.append("/").append(project).append("/").append(logstore).append("/api/v1/write");
        httpHeader[CONTENT_MD5] = CalcMD5(body);
        map<string, string> parameterList;
        string host = GetSlsHost();
        SetCommonHeader(httpHeader, (int32_t)(body.length()), "");
        string signature = GetUrlSignature(HTTP_POST, operation, httpHeader, parameterList, body, accessKeySecret);
        httpHeader[AUTHORIZATION] = LOG_HEADSIGNATURE_PREFIX + accessKeyId + ':' + signature;
        return make_unique<HttpSinkRequest>(HTTP_POST, mUsingHTTPS, host, mPort, operation, "", httpHeader, body, item);
    }

    unique_ptr<HttpSinkRequest>
    Client::CreateAsynPostLogStoreLogsRequest(const std::string& project,
                                              const std::string& logstore,
                                              const std::string& body,
                                              std::map<std::string, std::string>& httpHeader,
                                              const std::string& hashKey,
                                              int64_t hashKeySeqID,
                                              SenderQueueItem* item) {
        SLSClientManager::AuthType type;
        string accessKeyId, accessKeySecret;
        if (!SLSClientManager::GetInstance()->GetAccessKey(mAliuid, type, accessKeyId, accessKeySecret)) {
#ifdef __ENTERPRISE__
            static auto manager = static_cast<EnterpriseSLSClientManager*>(SLSClientManager::GetInstance());
            auto status = manager->GetProjectAnonymousWriteStatus(project);
            if (status == EnterpriseSLSClientManager::ProjectAnonymousWriteStatus::INVALID
                || status == EnterpriseSLSClientManager::ProjectAnonymousWriteStatus::PENDING) {
                return nullptr;
            }
            SLSClientManager::GetInstance()->GetAccessKey(
                STRING_FLAG(default_aliuid), type, accessKeyId, accessKeySecret);
#endif
        }
        if (type == SLSClientManager::AuthType::ANONYMOUS) {
            httpHeader[X_LOG_KEYPROVIDER] = MD5_SHA1_SALT_KEYPROVIDER;
        }

        string operation = LOGSTORES;
        operation.append("/").append(logstore);
        if (hashKey.empty())
            operation.append("/shards/lb");
        else
            operation.append("/shards/route");

        httpHeader[CONTENT_MD5] = CalcMD5(body);

        map<string, string> parameterList;
        if (!hashKey.empty()) {
            parameterList["key"] = hashKey;
            if (hashKeySeqID != kInvalidHashKeySeqID) {
                parameterList["seqid"] = std::to_string(hashKeySeqID);
            }
        }

        string host = GetHost(project);
        SetCommonHeader(httpHeader, (int32_t)(body.length()), project);
        string signature = GetUrlSignature(HTTP_POST, operation, httpHeader, parameterList, body, accessKeySecret);
        httpHeader[AUTHORIZATION] = LOG_HEADSIGNATURE_PREFIX + accessKeyId + ':' + signature;

        string queryString;
        GetQueryString(parameterList, queryString);

        return make_unique<HttpSinkRequest>(
            HTTP_POST, mUsingHTTPS, host, mPort, operation, queryString, httpHeader, body, item);
    }

    PostLogStoreLogsResponse
    Client::PingSLSServer(const std::string& project, const std::string& logstore, std::string* realIpPtr) {
        sls_logs::LogGroup logGroup;
        logGroup.set_source(LoongCollectorMonitor::mIpAddr);
        auto serializeData = logGroup.SerializeAsString();

        std::map<string, string> httpHeader;
        httpHeader[CONTENT_TYPE] = TYPE_LOG_PROTOBUF;
        httpHeader[X_LOG_BODYRAWSIZE] = std::to_string(serializeData.size());
        return SynPostLogStoreLogs(project, logstore, serializeData, httpHeader, "", realIpPtr);
    }

    PostLogStoreLogsResponse Client::SynPostLogStoreLogs(const std::string& project,
                                                         const std::string& logstore,
                                                         const std::string& body,
                                                         std::map<std::string, std::string>& httpHeader,
                                                         const std::string& hashKey,
                                                         std::string* realIpPtr) {
        string operation = LOGSTORES;
        operation.append("/").append(logstore);
        if (hashKey.empty())
            operation.append("/shards/lb");
        else
            operation.append("/shards/route");

        httpHeader[CONTENT_MD5] = CalcMD5(body);

        map<string, string> parameterList;
        if (!hashKey.empty())
            parameterList["key"] = hashKey;

        HttpMessage httpResponse;
        SendRequest(project, HTTP_POST, operation, body, parameterList, httpHeader, httpResponse, realIpPtr);

        PostLogStoreLogsResponse ret;
        ret.bodyBytes = (int32_t)body.size();
        ret.statusCode = httpResponse.statusCode;
        ret.requestId = httpResponse.header[X_LOG_REQUEST_ID];
        return ret;
    }

    PostLogStoreLogsResponse Client::SynPostMetricStoreLogs(const std::string& project,
                                                            const std::string& logstore,
                                                            const std::string& body,
                                                            std::map<std::string, std::string>& httpHeader,
                                                            std::string* realIpPtr) {
        string operation = METRICSTORES;
        operation.append("/").append(project).append("/").append(logstore).append("/api/v1/write");
        httpHeader[CONTENT_MD5] = CalcMD5(body);
        map<string, string> parameterList;
        HttpMessage httpResponse;
        SendRequest(project, HTTP_POST, operation, body, parameterList, httpHeader, httpResponse, realIpPtr);
        PostLogStoreLogsResponse ret;
        ret.bodyBytes = (int32_t)body.size();
        ret.statusCode = httpResponse.statusCode;
        ret.requestId = httpResponse.header[X_LOG_REQUEST_ID];
        return ret;
    }

    PostLogStoreLogsResponse Client::PostLogUsingWebTracking(const std::string& project,
                                                             const std::string& logstore,
                                                             sls_logs::SlsCompressType compressType,
                                                             const std::string& compressedLog,
                                                             uint32_t rawSize) {
        map<string, string> httpHeader;
        httpHeader[X_LOG_COMPRESSTYPE] = Client::GetCompressTypeString(compressType);
        httpHeader[X_LOG_BODYRAWSIZE] = std::to_string(rawSize);
        SetCommonHeader(httpHeader, (int32_t)(compressedLog.length()), project);

        string operation = LOGSTORES;
        operation.append("/").append(logstore).append("/track");

        string host = GetHost(project);
        int32_t port = mPort;
        if (mPort == 80 && mUsingHTTPS) {
            port = 443;
        }

        HttpMessage httpResponse;
        mClient->Send(HTTP_POST,
                      host,
                      port,
                      operation,
                      "",
                      httpHeader,
                      compressedLog,
                      mTimeout,
                      httpResponse,
                      AppConfig::GetInstance()->GetBindInterface(),
                      mUsingHTTPS);

        PostLogStoreLogsResponse ret;
        ret.bodyBytes = (int32_t)compressedLog.length();
        ret.statusCode = httpResponse.statusCode;
        ret.requestId = httpResponse.header[X_LOG_REQUEST_ID];
        return ret;
    }

    void Client::SetCommonHeader(map<string, string>& httpHeader, int32_t contentLength, const string& project) {
        if (!project.empty()) {
            httpHeader[HOST] = project + GetHostFieldSuffix();
        } else {
            httpHeader[HOST] = GetSlsHost();
        }

        httpHeader[USER_AGENT] = SLSClientManager::GetInstance()->GetUserAgent();
        httpHeader[X_LOG_APIVERSION] = LOG_API_VERSION;
        httpHeader[X_LOG_SIGNATUREMETHOD] = HMAC_SHA1;
        httpHeader[DATE] = GetDateString();
        httpHeader[CONTENT_LENGTH] = std::to_string(contentLength);
    }

    std::string Client::GetCompressTypeString(sls_logs::SlsCompressType compressType) {
        switch (compressType) {
            case sls_logs::SLS_CMP_NONE:
                return "";
            case sls_logs::SLS_CMP_LZ4:
                return LOG_LZ4;
            case sls_logs::SLS_CMP_ZSTD:
                return LOG_ZSTD;
            default:
                return LOG_LZ4;
        }
    }

    sls_logs::SlsCompressType Client::GetCompressType(std::string compressTypeString,
                                                      sls_logs::SlsCompressType defaultType) {
        if (compressTypeString == "none") {
            return sls_logs::SLS_CMP_NONE;
        } else if (compressTypeString == LOG_LZ4) {
            return sls_logs::SLS_CMP_LZ4;
        } else if (compressTypeString == LOG_ZSTD) {
            return sls_logs::SLS_CMP_ZSTD;
        }
        return defaultType;
    }
} // namespace sdk
} // namespace logtail
