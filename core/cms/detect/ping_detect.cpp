#if defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h> // 解决: 1) apr_sockaddr_t::sin6 使用未定义的struct sockaddr_in6。 2） LPMSG
#endif
#include "ping_detect.h"
#include "detect_ping_poll.h"

#include <cstring>
#include <memory>

#include "common/Logger.h"
#include "common/NetWorker.h"
#include "common/Nop.h"
#include "common/Random.h"
#include "common/Lazy.h"
#include "common/Common.h" // GetPid

#include <apr-1/apr_network_io.h>
#include "protocol_adt.h"

using namespace std;
using namespace common;
using namespace std::chrono;

static Lazy<DetectPingPoll> detectPingPoll;

namespace common
{
PingDetect::PingDetect(const string &destHost, const std::string &taskId,
                       const std::chrono::seconds &interval,
                       const std::chrono::microseconds &timeout) {
    mTaskId = taskId;
    mDestHost = destHost;
    // mPool = nullptr;
    mSock = std::make_shared<NetWorker>("PingDetect", timeout);
    // mRtt = std::chrono::microseconds{-1};
    mTimeout = timeout;
    mInterval = interval;
    mPingResult.windowSharePtr = std::make_shared<SlidingTimeWindow<int64_t>>(interval);
    mPingResult.count = 0;
    mPingResult.lostCount = 0;
    mPingResult.targetHost = destHost;
    mPingResult.lastScheduleTime = steady_clock::now(); // Now<ByMicros>();
    mPingResult.receiveEchoPackage = false;
	mPid = GetPid();
    //srand(apr_time_now());
    //mUuid = rand() ^ apr_time_now() & 0xffffffff;
	mUuid = Random<decltype(mUuid)>().next();
}

PingDetect::~PingDetect() {
    if (mSock) {
        detectPingPoll->DelSocket(*this);
    }

    mSock.reset();
}

bool PingDetect::Init(bool enablePoll) {
    if (0 == mSock->openIcmp()) {
        mDestSockaddr = mSock->createAddr(mDestHost.c_str(), 0);
    }
    bool success = (mDestSockaddr != nullptr);
    if (enablePoll && success) {
        success = (0 == detectPingPoll->AddSocket(*this));
    }

    return success;
}

bool PingDetect::PingSend() {
    ++mPingResult.count;
    mPingResult.lastScheduleTime = steady_clock::now(); // Now<ByMicros>();
    mPingResult.receiveEchoPackage = false;

    //not create socket each ping;
    if (!mDestSockaddr) {
        return false;
    }

    char sendBuf[128];
    memset(sendBuf, 0, sizeof(sendBuf));
    mSeq = GetSeq(mDestHost);
    //package icmp
    IcmpPack((struct icmp *) sendBuf, mPid, mSeq, 64);
    size_t sendLen = 64;
    mStartTime = steady_clock::now();
    int rv = mSock->sendTo(mDestSockaddr, 0, sendBuf, sendLen);
    if (rv != APR_SUCCESS) {
        LogWarn("send icmp msg error, destination: <{}>, error: ({}){}", mDestHost, rv, NetWorker::ErrorString(rv));
    }
    return rv == APR_SUCCESS;
}

bool PingDetect::PingReceive() {
    char recvBuf[512] = {0};
    size_t recvLen = sizeof(recvBuf) / sizeof(recvBuf[0]);
    // apr_status_t rt = apr_socket_recv(mSock->getSock(), recvBuf, &recvLen);
    int rt = mSock->recv(recvBuf, recvLen, false);
    if (rt != APR_SUCCESS || recvLen == 0) {
        LogWarn("PingReceive({}), ret: {}({}),  recvLen: {}", mDestHost, rt, NetWorker::ErrorString(rt), recvLen);
        return false;
    }
    return IcmpUnPack(mPid, recvBuf, recvLen);
}

int PingDetect::readEvent()
{
    if (PingReceive()) {
        if (!mPingResult.receiveEchoPackage && steady_clock::now() > mPingResult.lastScheduleTime + mTimeout) {
            LogWarn("PING[{}] TaskID ({}) timeout {} reached", mDestHost, mTaskId, mTimeout);
            return -1;
        }
        // 正确解包且是与发送包序列号匹配
        if (mPingResult.receiveEchoPackage) {
            // icmp 同一个包可能会收到多次，取第一次的包计算rt
            return -2;
        }

        mPingResult.receiveEchoPackage = true;
        auto rtt = steady_clock::now() - mPingResult.lastScheduleTime;
        mPingResult.windowSharePtr->Update(duration_cast<microseconds>(rtt).count());
        LogDebug("PING[{}][{}] TaskId: {}, schedule unit ping, receive icmp package, count={}",
                 mDestHost, mPingResult.count, mTaskId, mPingResult.count);
        return 0;
    }
    return -3;
}

static unsigned short CheckSum(unsigned short *addr, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;
    //把ICMP报头二进制数据以2字节为单位累加起来
    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }
    //若ICMP报头为奇数个字节，会剩下最后一字节。把最后一个字节视为一个2字节数据的高字节，这个2字节数据的低字节为0，继续累加
    if (nleft == 1)
    {
        *(unsigned char *) (&answer) = *(unsigned char *) w;
        sum += answer;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

void PingDetect::IcmpPack(struct icmp *icmpHdr, int pid, int seq, int length) const {
    icmpHdr->icmp_type = ICMP_ECHO;
    icmpHdr->icmp_code = 0;
    icmpHdr->icmp_cksum = 0;
    icmpHdr->icmp_seq = seq;
    icmpHdr->icmp_id = pid & 0xffff;
    icmpHdr->data[0] = mUuid;
    char *buf = (char *) icmpHdr;
    char *datapart = buf + sizeof(icmp);
    // Place some junk in the buffer
    memset(datapart, 'E', length - sizeof(icmp));
    icmpHdr->icmp_cksum = CheckSum((unsigned short *) icmpHdr, length);
}

bool PingDetect::IcmpUnPack(int pid, char *buf, size_t len) {
    auto *ipHeader = (struct ip *) buf;
    size_t iphdrLen = size_t(ipHeader->ip_hl) * 4;
    auto *icmp = (struct icmp *) (buf + iphdrLen);
    if (len < iphdrLen + 8)     //判断长度是否为ICMP包长度
    {
        LogWarn("PING[{}][{}], taskId: {}, invalid icmp packet. Its length {} is less than 8",
                mDestHost, mPingResult.count, mTaskId, len);
        return false;
    }

    len -= iphdrLen; // icmp包长度
    //判断该包是ICMP回送回答包且该包是我们发出去的
    if ((icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == (pid & 0xffff))) {
        if (icmp->icmp_seq != mSeq) {
            LogDebug("PING[{}][{}], taskId: {}, icmp packet seq is not right, {}:{}",
                     mDestHost, mPingResult.count, mTaskId, icmp->icmp_seq, mSeq);
            return false;
        }
        if (len < 12 || mUuid != icmp->data[0]) {
            LogWarn("PING[{}][{}], taskId: {}, icmp packet uuid is not right,{}:{}",
                    mDestHost, mPingResult.count, mTaskId, icmp->data[0], mUuid);
            return false;
        }
        /*
        * 尝试通过判断发送时icmp目的地址与接收时icmp源地址是否匹配，来判断是否为所需的icmp回包，在vpn链路下此方法会导致通畅链路失败（接收时icmp源地址会变为本机ip）。
        * 错误例子：https://aone.alibaba-inc.com/v2/project/460851/bug/43845049
        *         https://aone.alibaba-inc.com/v2/project/460851/bug/43931942
        */
        /*
        if( srcIp != mDestIp)
        {
            in_addr destIpAddr;
            in_addr srcIpAddr;
            destIpAddr.s_addr=mDestIp;
            srcIpAddr.s_addr=srcIp;
            char destIpStr[IP_STRING_LEN],srcIpStr[IP_STRING_LEN];
            strncpy(destIpStr,inet_ntoa(destIpAddr),IP_STRING_LEN);
            strncpy(srcIpStr,inet_ntoa(srcIpAddr),IP_STRING_LEN);
            destIpStr[IP_STRING_LEN-1]='\0';
            srcIpStr[IP_STRING_LEN-1]='\0';
            LogPrintInfo("icmp packet ip is not right, host ip:%s, packet src ip:%s", destIpStr, srcIpStr);
            return false;
        }
        */
        // int64_t now = NowMicros();
        auto rtt = duration_cast<fraction_millis>(steady_clock::now() - mStartTime);
        LogDebug("PING[{}][{}] TaskId: {}, {} bytes from {}: icmp_seq={} ttl={} rtt={:.1}", mDestHost,
                 mPingResult.count, mTaskId, len, IPv4String(ipHeader->ip_src), icmp->icmp_seq, ipHeader->ip_ttl, rtt);
        return true;
    } else {
        LogDebug("PING[{}][{}] TaskId: {}, invalid ICMP packet! Its id is not matched!",
                 mDestHost, mPingResult.count, mTaskId);
        return false;
    }
}

// string PingDetect::GetIpString(uint32_t num) {
//     string strRet[4];
//     for (int i = 0; i < 4; i++) {
//         uint32_t tmp = (num >> ((3 - i) * 8)) & 0xFF;
//         char chBuf[8] = "";
//         sprintf(chBuf, "%d", tmp);
//         strRet[i] = chBuf;
//     }
//     return (strRet[3] + "." + strRet[2] + "." + strRet[1] + "." + strRet[0]);
// }
// std::string PingDetect::GetIpString(uint32_t num) {
//     const auto *chr = (const uint8_t *)&num;
//     return fmt::format("{}.{}.{}.{}", chr[0], chr[1], chr[2], chr[3]);
// }

static std::atomic<uint16_t> pingSeq(1);

uint16_t PingDetect::GetSeq(const string &) {
    return pingSeq += 2;
}

string const &PingDetect::GetTaskId() const {
    return mTaskId;
}

PingResult &PingDetect::GetPingResult() {
    return mPingResult;
}

bool PingDetect::CleanResult() {
    mPingResult.count = 0;
    mPingResult.lostCount = 0;
    mPingResult.lastScheduleTime = steady_clock::time_point{};
    mPingResult.receiveEchoPackage = false;
    mPingResult.windowSharePtr = std::make_shared<SlidingTimeWindow<int64_t>>(mInterval);
    return true;
}

microseconds PingDetect::GetTimeout() const {
    return mTimeout;
}

bool PingDetect::AddTimeoutItem() {
    mPingResult.receiveEchoPackage = true;
    ++mPingResult.lostCount;
    // 超时不计算 rt
    return true;

}
} // namespace cloudMonitor
