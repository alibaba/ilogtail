#ifndef _DOMAIN_SOCKET_LISTEN_H_
#define _DOMAIN_SOCKET_LISTEN_H_

#include <list>
#include <memory>
#include "common/PollEventBase.h"

namespace common {
    class NetWorker;
}

namespace argus {
    class DomainSocketCollect;

    class DomainSocketListen : public common::PollEventBase {
        friend class DomainSocketCollect;
    private:
        std::shared_ptr<common::NetWorker> m_net;
        std::weak_ptr<DomainSocketCollect> m_collector;
    public:
        DomainSocketListen();
        ~DomainSocketListen() override;

        int listen(const char *sockPath, uint16_t localPort, const std::shared_ptr<DomainSocketCollect> &c);
        int readEvent(const std::shared_ptr<PollEventBase> &myself) override;
    };
}
#endif
