#ifndef _HTTP_METRIC_CLIENT_H_
#define _HTTP_METRIC_CLIENT_H_
#include <atomic>
#include <memory>

#include "common/PollEventBase.h"
#include "common/HttpServer.h"

struct apr_socket_t;
namespace common {
    class NetWorker;
}

namespace argus
{
class HttpMetricClient: public common::PollEventBase
{
public:
    explicit HttpMetricClient(const std::shared_ptr<common::NetWorker> &pNet);
    ~HttpMetricClient() override;
    int readEvent(const std::shared_ptr<PollEventBase> &myself) override;
    int SendResponse(const common::HttpServerMsg &response);
    const common::HttpServerMsg &GetHttpServerMsg() const { return mRequest; }
    // bool IsRunning() const;
private:
    void DoClose();
private:
    // std::atomic_bool mRunning{false};
    std::atomic_bool mRequestStart{false};
    // common::InstanceLock mNetLock;
    std::shared_ptr<common::NetWorker> mNet;
    common::HttpServerMsg mRequest;
    common::HttpServerMsg mTmpRequest;
};
}
#endif