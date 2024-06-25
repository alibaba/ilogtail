#include "http_metric_client.h"
#include "argus_manager.h"
#include "http_metric_collect.h"

#include "common/CPoll.h"
#include "common/NetWorker.h"
#include "common/TLVHandler.h"
#include "common/Logger.h"

using namespace common;

namespace argus
{
HttpMetricClient::HttpMetricClient(const std::shared_ptr<common::NetWorker> &pNet): mNet(pNet)
{
    mRequestStart = false;
    // mRunning = true;
}
HttpMetricClient::~HttpMetricClient()
{
    LogDebug("~HttpMetricClient({})", (void *)this);
    DoClose();
}

int HttpMetricClient::readEvent(const std::shared_ptr<PollEventBase> &myself)
{
    // Sync(mNet) {
        RecvState rs = HttpServer::ReceiveRequest(mNet.get(), mTmpRequest);
        LogDebug("rs = {}({})", (int)rs, (rs == S_Error ? "S_Error" : (rs == S_Incomplete ? "S_Incomplete" : "OK")));
        if (rs == S_Error) {
            if (mRequestStart) {
                LogInfo("the client closed,as the response is not sent to the client!");
            }
            DoClose();
            return 0;
        } else if (rs == S_Incomplete) {
            return 0;
        }
    // }}}
    if(mRequestStart)
    {
        //已经在处理了请求，每个连接只能处理一个请求
        LogInfo("only one request can't be dealed!");
        return 0;
    }
    //数据处理,加入到metricProcess线程
    mRequestStart = true;
    mRequest = mTmpRequest;
    mTmpRequest = HttpServerMsg{};
    SingletonHttpMetricCollect::Instance()->AddHttpMetricRequest(std::dynamic_pointer_cast<HttpMetricClient>(myself));
    return 0;
}
void HttpMetricClient::DoClose()
{
    // Sync(mNet) {
    //     if (!mNet) {
    //         LogInfo("already delete");
    //         return 0;
    //     }
        // apr_socket_t *pSock = mNet->getSock();
        // if (SingletonPoll::Instance()->delSocket(pSock) != 0) {
        //     LogError("Delete http script runner from poll failed");
        // }
    // delete mNet;
    // mNet = nullptr;
    if (mNet) {
        mNet->shutdown();
        mNet.reset();
    }
        // mRunning = false;
    // }}}
	// return 0;
}
int HttpMetricClient::SendResponse(const HttpServerMsg &response)
{
    // Sync(mNet) {
    //     if (mNet) {
            HttpServer::SendResponse(mNet.get(), response);
    //     }
    // }}}
    mRequestStart = false;
	return 0;
}
// bool HttpMetricClient::IsRunning() const
// {
//     if(!mRequestStart && !mNet)
//     {
//         return false;
//     }else
//     {
//         //进一步判断
//         return true;
//     }
// }
}