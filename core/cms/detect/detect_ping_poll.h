//
// Created by 许刘泽 on 2022/8/12.
//

#ifndef ARGUSAGENT_SRC_COMMON_DETECT_PING_POLL_H_
#define ARGUSAGENT_SRC_COMMON_DETECT_PING_POLL_H_

#include <map>
#include <atomic>
#include "common/ThreadWorker.h"
#include "common/SafeMap.h"

struct apr_pool_t;
struct apr_pollset_t;
struct apr_socket_t;

namespace common {
    class PingDetect;

    class DetectPingPoll : public ThreadWorker {
    public:
        DetectPingPoll();
        ~DetectPingPoll() override;
        int AddSocket(PingDetect &pSock);
        int DelSocket(PingDetect &pSock);
    private:
        void doRun() override;
        int CreatePollSet();

        int UpdateSocket(apr_socket_t *pSock, PingDetect *pObject);
    private:
        apr_pool_t *mPool = nullptr;
        apr_pollset_t *mPollSet = nullptr;
        // InstanceLock mSockObjectMapLock;
        SafeMap<apr_socket_t *, PingDetect *> mSockObjectMap;
    };
}

#endif //ARGUSAGENT_SRC_COMMON_DETECT_PING_POLL_H_
