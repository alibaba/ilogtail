#if defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h> // 解决apr_sockaddr_t::sin6 使用未定义的struct sockaddr_in6
#endif

#include "common/NetWorker.h"
#include "common/PollEventBase.h"
#include "common/CPoll.h"
#include "common/Logger.h"
#include "common/Common.h"
#include "common/TimeProfile.h"
#include "common/ResourceConsumptionRecord.h"

#include <apr-1/apr_poll.h>

#ifdef WIN32
#include <stdlib.h> // _exit
#else

#include <unistd.h> // _exit

#endif

#include "common/Config.h"

using namespace common;

Poll::Poll() {
    static_assert(sizeof(void *) == sizeof(uintptr_t), "sizeof(int) is not less than or equal to sizeof(void *)");

    apr_pool_create(&m_p, nullptr);
    if (createPollSet() != 0) {
        LogError("createPoolSet != 0, program exit");
        _exit(1);
    }
}

Poll::~Poll() {
    endThread();
    join();
    Sync(m_sockObjectMap) {
        for (auto &it: m_sockObjectMap) {
            removeFromPoll(it.second, it.first, "~Poll");
        }
        m_sockObjectMap.clear();
    }}}
    apr_pollset_destroy(m_pollset);
    apr_pool_destroy(m_p);
}

apr_pollset_t *Poll::createPollSet(apr_pool_t *pool, uint32_t size) {
    apr_pollset_t *pollSet = nullptr;
    apr_status_t rv = apr_pollset_create(&pollSet, size, pool, APR_POLLSET_THREADSAFE);
    if (rv == APR_ENOTIMPL) {
        rv = apr_pollset_create(&pollSet, size, pool, 0);
    }
    if (rv != APR_SUCCESS) {
        LogError("apr_pollset_create({}) error. ({}) {}", size, rv, NetWorker::ErrorString(rv));
        pollSet = nullptr;
    }
    return pollSet;
}

int Poll::createPollSet() {
    m_pollset = createPollSet(m_p, 1000);
    return m_pollset ? 0 : -1;
}

uintptr_t Poll::find(PollEventBase *eventp) const {
    if (eventp) {
        Sync(m_sockObjectMap) {
            for (const auto &it: m_sockObjectMap) {
                if (it.second.event.get() == eventp) {
                    return it.first;
                }
            }
        }}}
    }
    return 0;
}

struct AprPollFdEx : apr_pollfd_t {
    explicit AprPollFdEx(apr_socket_t *socket, uintptr_t n) : apr_pollfd_t() {
        this->p = nullptr;
        this->reqevents = APR_POLLIN | APR_POLLERR | APR_POLLHUP;
        this->rtnevents = 0;
        this->desc_type = APR_POLL_SOCKET;
        this->desc.s = socket;
        this->client_data = reinterpret_cast<void *>(n);
    }
};

int Poll::add(const std::shared_ptr<PollEventBase> &pObject, const std::shared_ptr<NetWorker> &net) {
    if (net == nullptr || net->getSock() == nullptr || pObject == nullptr) {
        LogError("Poll::addSocket skipped, PollEventBase Object {}, pSock {}",
                 (pObject == nullptr ? "null" : typeName(pObject.get())),
                 (net == nullptr || net->getSock() == nullptr ? "null" : "ok"));
        return -1;
    }

    Sync(m_sockObjectMap) {
        if (find(pObject.get())) {
            LogWarn("socket already in pollset");
            return -1;
        }

        uintptr_t key = 0;
        // 避免碰撞
        do {
            // 从1起，每次+2，则key必然不会出现偶数，即不可能为0
            key = this->counter.fetch_add(2);
        } while (m_sockObjectMap.find(key) != m_sockObjectMap.end());

        const std::string connection = net->toString();

        AprPollFdEx pfd(net->getSock(), key);
        apr_status_t rv = apr_pollset_add(m_pollset, &pfd);
        if (rv != APR_SUCCESS) {
            LogError("add ({}@{}, {}) to pollset error: ({}){}",
                     typeName(pObject.get()), (void *) pObject.get(), connection, rv,
                     NetWorker::ErrorString(rv));
            return -1;
        }

        m_sockObjectMap[key] = tagItem{pObject, net};

        LogInfo("add[{}] ({}@{}, {}) to pollset ok, left: {}",
                key, typeName(pObject.get()), (void *) pObject.get(), connection, m_sockObjectMap.size());
        return 0;
    }}}
}

// 避免递归删除: m_sockObjectMap.erase()时，有可能触发shared_ptr<PollEventBase>的析构remove
// #define ENABLE_THREAD_LOCAL
#ifdef ENABLE_THREAD_LOCAL
static thread_local PollEventBase *removing = nullptr;
#endif

int Poll::removeFromPoll(const tagItem &item, uintptr_t key, const std::string &op) {
    if (auto *sock = item.net->getSock()) {
        AprPollFdEx pfd(sock, key);
        apr_status_t rv = apr_pollset_remove(m_pollset, &pfd);
        if (rv != APR_SUCCESS && rv != APR_NOTFOUND) {
            const std::string connection = item.net->toString();
            std::string name = item.event ? typeName(item.event.get()) : std::string{"<unknown>"};
            LogError("[{}]remove[{}] ({}@{}, {}) from pollset error. ({}){}",
                     op, key, name, (void *) item.event.get(), connection, rv, getErrorString(rv));
            return -1;
        }
    }
    return 0;
}

int Poll::remove(uintptr_t key, const std::string &op) {
    Sync(m_sockObjectMap) {
        auto it = m_sockObjectMap.find(key);
        if (it == m_sockObjectMap.end()) {
            LogWarn("[{}]no entry key({}) to remove in pollset", op, key);
            return -1;
        }
        // 先缓存一下，否则后面m_sockObjectMap.erase时，可能会触发event的析构，从而递归调用该函数，从而产生二次删除。
        auto second = it->second;
#ifdef ENABLE_THREAD_LOCAL
        removing = second.event.get();
        defer(removing = nullptr);
#endif
        const auto *ptr = second.event.get();
        const std::string objName = typeName(ptr);
        const std::string connection = second.net->toString();

        if (0 != removeFromPoll(second, key, op)) {
            return -1;
        }
        // AprPollFdEx pfd(it->second.net->getSock(), key);
        // apr_status_t rv = apr_pollset_remove(m_pollset, &pfd);
        // if (rv != APR_SUCCESS && rv != APR_NOTFOUND) {
        //     LogError("[{}]remove[{}] ({}@{}, {}) from pollset error. ({}){}",
        //              op, key, objName, (void *) it->second.event.get(), connection, rv, getErrorString(rv));
        //     return -1;
        // }

        m_sockObjectMap.erase(key);
        // 到此处，second就可以安全的析构了
        LogInfo("[{}]remove[{}] ({}@{}, {}) from pollset ok, left: {}",
                op, key, objName, (void *) ptr, connection, m_sockObjectMap.size());
    }}}
    return 0;
}

int Poll::remove(PollEventBase *event, const std::string &op) {
    int rv = APR_SUCCESS;
#ifdef ENABLE_THREAD_LOCAL
    if (event != removing) {
#endif
        auto key = find(event);
        if (!key) {
            LogDebug("[{}]remove[?] {}@{} failed: no entry in pollset", op, typeName(event), (void *) event);
            rv = -1;
        } else {
            rv = remove(key, op);
        }
#ifdef ENABLE_THREAD_LOCAL
    }
#endif
    return rv;
    // auto key = makeKey(event);
    // Sync(m_sockObjectMap) {
    //     auto it = m_sockObjectMap.find(key);
    //     if (it == m_sockObjectMap.end()) {
    //         LogWarn("[{}]no socket({}) to delete in pollset", op, (void *)event);
    //         return -1;
    //     }
    //
    //     AprPollFdEx pfd(it->second->getSock(), it->first.get());
    //     apr_status_t rv = apr_pollset_remove(m_pollset, &pfd);
    //     if (rv != APR_SUCCESS && rv != APR_NOTFOUND) {
    //         LogError("[{}]delete socket({}) from pollset error. ({}){}",
    //                  op, (void *)event, rv, getErrorString(rv));
    //         return -1;
    //     }
    //
    //     m_sockObjectMap.erase(key);
    //     LogDebug("[{}]delete tcp connection ({}) from pollset ok, left: {}",
    //              op, (void *)event, m_sockObjectMap.size());
    // }}}
    // return 0;
}

// int Poll::getSocketCount()
// {
//     m_sockObjectMapLock.lock();
//     int count = m_sockObjectMap.size();
//     m_sockObjectMapLock.unlock();
//     return count;
// }

void Poll::doRun() {
    auto event2Str = [](int16_t event) {
#define VAL_NAME(V) {V, #V}
        struct {
            int16_t mask;
            const char *name;
        } masks[] = {
                VAL_NAME(APR_POLLIN),     /**< Can read without blocking */
                VAL_NAME(APR_POLLPRI),    /**< Priority data available */
                VAL_NAME(APR_POLLOUT),    /**< Can write without blocking */
                VAL_NAME(APR_POLLERR),    /**< Pending error */
                VAL_NAME(APR_POLLHUP),    /**< Hangup occurred */
                VAL_NAME(APR_POLLNVAL),   /**< Descriptor invalid */
        };
#undef VAL_NAME
        std::stringstream ss;
        const char *sep = "";
        for (const auto &it: masks) {
            if (event & it.mask) {
                ss << sep << it.name;
                sep = " | ";
            }
        }
        return ss.str();
    };

    auto timeout = SingletonConfig::Instance()->GetValue<int64_t>("agent.poll.timeout", 10 * 1000);
    LogInfo("Poll::doRun(), timeout: {} us", convert(timeout, true));
    while (!isThreadEnd()) {
        apr_int32_t num = 0;
        const apr_pollfd_t *ret_pfd = nullptr;
        apr_status_t rv = apr_pollset_poll(m_pollset, timeout, &num, &ret_pfd);

        if (APR_SUCCESS == rv) {
            CpuProfile("Poll::doRun");

            Sync(m_sockObjectMap) {
                for (int i = 0; i < num; i++) {
                    const auto key = reinterpret_cast<uintptr_t>(ret_pfd[i].client_data);
                    if (ret_pfd[i].rtnevents & (APR_POLLIN | APR_POLLERR)) {
                        const auto it = m_sockObjectMap.find(key);
                        if (it != m_sockObjectMap.end()) {
                            auto &client = it->second;
                            if (client.net && (ret_pfd[i].rtnevents & APR_POLLHUP)) {
                                LogDebug("Poll::doRun({}), {}@{}, {}", client.net->toString(),
                                         typeName(client.event.get()), (void *) client.event.get(),
                                         event2Str(ret_pfd[i].rtnevents));
                            }
                            client.event->readEvent(client.event);
                        }
                    }
                    if (ret_pfd[i].rtnevents & APR_POLLHUP) {
                        remove(key, "Poll::doRun");
                    }
                }
            }}}
        } else {
            if (!APR_STATUS_IS_TIMEUP(rv) && !APR_STATUS_IS_EINTR(rv)) {
                LogError("poll error: ({}){}", rv, NetWorker::ErrorString(rv));
            }
            sleepFor(timeout);
        }
    }
}
