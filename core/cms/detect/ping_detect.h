#ifndef ARGUS_DETECT_PING_DETECT_H
#define ARGUS_DETECT_PING_DETECT_H

#include <string>
#include <chrono>
#include <memory>
#include "common/PollEventBase.h"
#include "detect_result.h"

struct apr_sockaddr_t;
struct ip;
struct icmp;

namespace common {
    class DetectPingPoll;
    class NetWorker;

#include "common/test_support"
class PingDetect {
    friend class DetectPingPoll;
public:
    PingDetect(const std::string &destHost, const std::string &taskId,
               const std::chrono::seconds &interval = std::chrono::seconds{60},
               const std::chrono::microseconds &timeout = std::chrono::microseconds{800 * 1000});
    VIRTUAL ~PingDetect();
    int readEvent();
    VIRTUAL bool PingSend();
    VIRTUAL bool PingReceive();
    VIRTUAL bool Init(bool enablePoll);
    bool Init() {
        return Init(true);
    }
    std::string const &GetTaskId() const;
    PingResult &GetPingResult();

    std::chrono::seconds GetInterval() const {
        return mInterval;
    }

    bool CleanResult();
    std::chrono::microseconds GetTimeout() const;
    bool AddTimeoutItem();

private:
    void IcmpPack(struct icmp *icmphdr, int pid, int seq, int length) const;
    bool IcmpUnPack(int pid, char *buf, size_t len);
    static uint16_t GetSeq(const std::string &);

    std::chrono::microseconds mTimeout{0};
    // std::chrono::microseconds mRtt{0};
    std::chrono::steady_clock::time_point mStartTime;
    std::string mDestHost;
    std::string mTaskId;
    // apr_socket_t *mSock = nullptr;
    // apr_pool_t *mPool = nullptr;
    apr_sockaddr_t *mDestSockaddr = nullptr;
    std::shared_ptr<common::NetWorker> mSock;
    int mPid = 0;
    uint16_t mSeq = 1;
    uint32_t mUuid = 1;
    std::chrono::seconds mInterval{15};
    PingResult mPingResult;
};
#include "common/test_support"

} // namespace cloudMonitor
#endif
