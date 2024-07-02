#ifndef _HTTP_METRIC_LISTEN_H_
#define _HTTP_METRIC_LISTEN_H_

#include <memory>
#include "common/NetWorker.h"
#include "common/PollEventBase.h"

namespace argus {
    class HttpMetricCollect;

    class HttpMetricListen : public common::PollEventBase {
        friend class HttpMetricCollect;
    public:
        HttpMetricListen();
        ~HttpMetricListen() override;
        int Listen();
        int readEvent(const std::shared_ptr<PollEventBase> &myself) override;

        void close();

        std::string getListenIp() const {
            return mListenIp;
        }
        int getListenPort() const {
            return mListenPort;
        }
    private:
        std::shared_ptr<common::NetWorker> mNet;
        int mMaxAccessRate = 100;
        int mListenPort = 15777;
        std::string mListenIp = "127.0.0.1";
    };
}
#endif
