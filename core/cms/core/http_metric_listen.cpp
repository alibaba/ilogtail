#include "http_metric_listen.h"
#include "http_metric_client.h"

#include "common/Logger.h"
#include "common/CPoll.h"
#include "common/Config.h"

using namespace common;

namespace argus {
    HttpMetricListen::HttpMetricListen() {
        mNet = std::make_shared<NetWorker>("HttpMetricListen");
        mListenIp = SingletonConfig::Instance()->GetValue("agent.http.metric.listen.ip", "127.0.0.1");
        mListenPort = SingletonConfig::Instance()->GetValue("agent.http.metric.listen.port", 15777);
        mMaxAccessRate = SingletonConfig::Instance()->GetValue("agent.http.metric.max.access.rate", 100);
        LogInfo("{}({}): agent.http.metric.listen.at={}:{}", __FUNCTION__, (void *) this, mListenIp, mListenPort);
        LogInfo("{}({}): agent.http.metric.max.access.rate={}", __FUNCTION__, (void *) this, mMaxAccessRate);
    }

    HttpMetricListen::~HttpMetricListen() {
        // SingletonPoll::Instance()->delSocket(this);
        // mNet.reset();
        LogInfo("{}({})", __FUNCTION__, (void *) this);
    }

    int HttpMetricListen::Listen() {
        mNet->setTimeout(3_s);
        if (mNet->listen(mListenIp, mListenPort, 10) != 0) {
            LogError("Create http script run listener failed({}:{})", mListenIp, mListenPort);
            return -1;
        }
        // apr_sockaddr_t *sa;
        // if (mNet.createAddr(&sa, mListenIp.c_str(), mListenPort) != 0) {
        //     LogError("create localAddr failed with {}:{}", mListenIp, mListenPort);
        //     mNet.clear();
        //     return -1;
        // }
        //
        // constexpr auto _3s = std::chrono::seconds{3};
        // if (mNet.socket() != 0 || mNet.setSockOpt(APR_SO_REUSEADDR, 1) != 0 || mNet.setSockTimeOut(_3s) != 0
        //     || mNet.bind(sa, "http metric listener") != 0 || mNet.listen(10) != 0) {
        //     LogError("Create http script run listener failed({}:{})", mListenIp, mListenPort);
        //     mNet.clear();
        //     return -1;
        // }

        // if (SingletonPoll::Instance()->addSocket(this) != 0) {
        //     LogError("Add http script run listener to poll failed");
        //     mNet.clear();
        //     return -1;
        // }
        // LogInfo("Add http script run listener to poll successful({}:{})", mListenIp, mListenPort);
        return 0;
    }

    void HttpMetricListen::close() {
        mNet->shutdown();
    }

    int HttpMetricListen::readEvent(const std::shared_ptr<PollEventBase> &myself) {
        // for (auto it = mClients.begin(); it != mClients.end();) {
        //     const auto &pClient = *it;
        //     if (!pClient->IsRunning()) {
        //         // delete pClient;
        //         it = mClients.erase(it);
        //     } else {
        //         it++;
        //     }
        // }
        std::shared_ptr<NetWorker> pNet;
        if (mNet->accept(pNet, "HttpMetricClient") != 0) {
            LogError("Accept client failed");
            // delete pNet;
            return -1;
        }
        SingletonPoll::Instance()->add<HttpMetricClient>(pNet);
        // auto pClient = std::make_shared<HttpMetricClient>(pNet);
        // if (SingletonPoll::Instance()->addSocket(pNet->getSock(), pClient) != 0) {
        //     LogWarn("Add a client to poll failed");
        //     mNet.clear();
        //     // delete pClient;
        //     return -1;
        // }
        // mClients.push_back(pClient);
        return 0;
    }
// bool HttpMetricListen::CheckAccessRate(NetWorker *pNet)
// {
//     int64_t now = NowMicros();
//     mAccessCount++;
//     if (now < mLastAccessTime || now - mLastAccessTime > 1000 * 1000)
//     {
//         mLastAccessTime = now;
//         mAccessCount = 1;
//         return true;
//     }
//     else if (mAccessCount > mMaxAccessRate)
//     {
//         int status = 500;
//         std::string content = "request too fast,please try slowly!";
//         HttpServerMsg msg = HttpServer::CreateHttpServerResponse(content,status);
//         HttpServer::SendResponse(pNet,msg);
//         LogWarn("Access Too Fast,exceed {} per second",mMaxAccessRate);
//         return false;
//     } else
//     {
//         return true;
//     }
// }
}