#ifndef _DOMAIN_SOCKET_CLIENT_H_
#define _DOMAIN_SOCKET_CLIENT_H_

#include <atomic>
#include <vector>
#include <memory>

#include "common/PollEventBase.h"
#include "common/TLVHandler.h"

namespace common {
    class NetWorker;
}
struct apr_socket_t;

namespace argus {
// #define PBMSG 0
#define MSG_HDR_LEN 8
// #define SHOW_TAG 0
// #define INSPECTOR_SAMPLING_INTERVAL 1000000
#define MAX_DOMAIN_SOCKET_PACKAGE_SIZE (10*1024*1024)
    struct MessageHdr {
        int32_t type = 0;
        uint32_t len = 0;
    };

    struct DomainSocketPacket {
        size_t mRecvSize = 0;
        char mPacketHdr[MSG_HDR_LEN] = {0};
        std::vector<char> mPacket;
        int32_t mPacketType = 0;
    };
    static_assert(sizeof(MessageHdr) == sizeof(DomainSocketPacket::mPacketHdr), "sizeof(MessageHdr) not matched");

    class DomainSocketCollect;

#include "common/test_support"
class DomainSocketClient : public common::PollEventBase {
public:
    DomainSocketClient(const std::shared_ptr<common::NetWorker> &pNet,
                       const std::shared_ptr<DomainSocketCollect> &fnCollector);
    ~DomainSocketClient() override;
    int readEvent(const std::shared_ptr<PollEventBase> &myself) override;
    apr_socket_t *getSock() const;
    // std::atomic_int m_run;

    bool isRunning() const {
        return m_net != nullptr;
    }

private:
    common::RecvState ReadDSPacketHdr();
    common::RecvState ReadDSPacketData();
    // int doClose();

private:
    std::shared_ptr<common::NetWorker> m_net;
    DomainSocketPacket m_package;
    int64_t m_PacketNum;
    std::weak_ptr<DomainSocketCollect> m_fnCollector;
};
#include "common/test_support"

}
#endif