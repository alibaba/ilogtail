//
// Created by 许刘泽 on 2022/8/12.
//
#if defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h> // 解决: 1) apr_sockaddr_t::sin6 使用未定义的struct sockaddr_in6。 2） LPMSG
#endif
#include "detect_ping_poll.h"
#include "ping_detect.h"

#include "common/Config.h"
#include "common/Common.h"  // getErrorString
#include "common/Logger.h"
#include "common/TimeProfile.h"
#include "common/Nop.h"
#include "common/ResourceConsumptionRecord.h"
#include "common/NetWorker.h"

#include "apr-1/apr_poll.h"

using namespace common;

DetectPingPoll::DetectPingPoll() {
    apr_pool_create(&mPool, nullptr);
    if (CreatePollSet() != 0) {
        LogError("CreatePoolSet != 0, program exit");
        _exit(1);
    }
}

DetectPingPoll::~DetectPingPoll() {
    LogInfo(">>> ~DetectPingPoll(@{})", (void *)this);
    endThread();
    if (mPollSet) {
        apr_pollset_wakeup(mPollSet);
    }
    join();

    apr_pollset_destroy(mPollSet);
    mPollSet = nullptr;
    apr_pool_destroy(mPool);
    LogInfo("    ~DetectPingPoll(@{}) >>>", (void *)this);
}

int DetectPingPoll::CreatePollSet() {
    apr_status_t rv = apr_pollset_create(&mPollSet, 1000, mPool, APR_POLLSET_THREADSAFE | APR_POLLSET_WAKEABLE);
    if (rv == APR_ENOTIMPL) {
        rv = apr_pollset_create(&mPollSet, 1000, mPool, 0);
    }
    if (rv != APR_SUCCESS) {
        LogError("create pollset error. ({}) {}", rv, getErrorString(rv));
        return -1;
    }
    return 0;
}

void DetectPingPoll::doRun() {
    auto timeout = SingletonConfig::Instance()->GetValue<int64_t>("agent.poll.timeout", 10 * 1000);
    while (!isThreadEnd()) {
        apr_int32_t num = 0;
        const apr_pollfd_t *pollfd = nullptr;
        apr_status_t rv = apr_pollset_poll(mPollSet, timeout, &num, &pollfd);
        if (APR_SUCCESS == rv) {
            CpuProfile("DetectPingPoll::doRun");

            // std::unique_lock<InstanceLock> uniqueLock{mSockObjectMapLock};
            Sync(mSockObjectMap) {
                for (int i = 0; i < num; i++) {
                    apr_socket_t *pSock = pollfd[i].desc.s;
                    auto targetSockIt = mSockObjectMap.find(pSock);
                    if (targetSockIt != mSockObjectMap.end()) {
                        if (auto *object = targetSockIt->second) {
                            object->readEvent();
                        }
                    }
                }
            }}}
        } else {
            if (!APR_STATUS_IS_TIMEUP(rv) && !APR_STATUS_IS_EINTR(rv)) {
                LogError("Poll error: ({}){}", rv, NetWorker::ErrorString(rv));
            }
        }
    }
}

int DetectPingPoll::UpdateSocket(apr_socket_t *pSock, PingDetect *pObject) {
    bool add = pObject != nullptr;
    const char *op = (add? "add": "delete");
    const char *direction = (add? "to": "from");

    // std::unique_lock<InstanceLock> uniqueLock{mSockObjectMapLock};
    Sync(mSockObjectMap) {
        if (add == (mSockObjectMap.find(pSock) != mSockObjectMap.end())) {
            LogPrintWarn((add ? "socket already in pollset" : "no socket to delete from pollset"));
            return -1;
        }

        apr_pollfd_t pfd;
        pfd.desc_type = APR_POLL_SOCKET;
        pfd.reqevents = APR_POLLIN | APR_POLLERR;
        pfd.desc.s = pSock;
        pfd.client_data = pSock;
        apr_status_t rv = (add ? apr_pollset_add : apr_pollset_remove)(mPollSet, &pfd);
        if (rv != APR_SUCCESS) {
            LogPrintError("%s socket %s pollset error: ({}){}", op, direction, rv, NetWorker::ErrorString(rv));
            return -1;
        }

        if (add) {
            if (mSockObjectMap.empty()) {
                runIt();
            }
            mSockObjectMap[pSock] = pObject;
        } else {
            mSockObjectMap.erase(pSock);
            if (mSockObjectMap.empty()) {
                endThread();
                join();
            }
        }
        LogPrintDebug("%s socket %s pollset ok, left:%d", op, direction, mSockObjectMap.size());
    }}}

    return 0;
}

int DetectPingPoll::AddSocket(PingDetect &pObject) {
    return UpdateSocket(pObject.mSock->getSock(), &pObject);
    // std::unique_lock<InstanceLock> uniqueLock{mSockObjectMapLock};
    // if (mSockObjectMap.find(pSock) != mSockObjectMap.end()) {
    //     LogPrintWarn("socket already in poolset");
    //     return -1;
    // }
    // apr_pollfd_t pfd;
    // pfd.desc_type = APR_POLL_SOCKET;
    // pfd.reqevents = APR_POLLIN | APR_POLLERR;
    // pfd.desc.s = pSock;
    // pfd.client_data = pSock;
    // apr_status_t rv = apr_pollset_add(mPollSet, &pfd);
    // if (rv != APR_SUCCESS) {
    //     LogPrintWarn("Error info : %s", getErrorString(rv).c_str());
    //     LogPrintError("add tcp connection to pollset error.");
    //     return -1;
    // }
    //
    // mSockObjectMap[pSock] = pObject;
    // LogPrintDebug("add socket to pollset ok, left:%d", mSockObjectMap.size());
    //
    // return 0;
}

int DetectPingPoll::DelSocket(PingDetect &detect) {
    return UpdateSocket(detect.mSock->getSock(), nullptr);
    // std::unique_lock<InstanceLock> uniqueLock{mSockObjectMapLock};
    // if (mSockObjectMap.find(pSock) == mSockObjectMap.end()) {
    //     LogPrintWarn("no socket to delete in poolset");
    //     return -1;
    // }
    //
    // apr_pollfd_t pfd;
    // pfd.desc_type = APR_POLL_SOCKET;
    // pfd.reqevents = APR_POLLIN | APR_POLLERR;
    // pfd.desc.s = pSock;
    // pfd.client_data = pSock;
    // apr_status_t rv = apr_pollset_remove(mPollSet, &pfd);
    // if (rv != APR_SUCCESS) {
    //     LogPrintWarn("Error info : %s", getErrorString(rv).c_str());
    //     LogPrintError("delete socket from pollset error.");
    //     return -1;
    // }
    //
    // mSockObjectMap.erase(pSock);
    //
    // LogPrintDebug("delete tcp connection from pollset ok, left:%d", mSockObjectMap.size());
    //
    // return 0;
}

