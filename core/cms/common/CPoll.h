#ifndef ARGUS_COMMON_CPOLL_H
#define ARGUS_COMMON_CPOLL_H

#include <cstdint> // uintptr_t
#include <atomic>
#include <memory>      // std::shared_ptr
#include <type_traits> // std::enable_if

#include "Singleton.h"
#include "ThreadWorker.h"
#include "NetWorker.h"
#include "Nop.h"
#include "PollEventBase.h"
#include "common/SafeMap.h"

struct apr_pool_t;
struct apr_pollset_t;
struct apr_socket_t;

namespace common {
    class Poll : public ThreadWorker {
    private:
        apr_pool_t *m_p = nullptr;
        apr_pollset_t *m_pollset = nullptr;

        std::atomic<uintptr_t> counter{1}; // 每次增2，以确保『绝对不会产生』0
        struct tagItem {
            std::shared_ptr<PollEventBase> event;
            std::shared_ptr<NetWorker> net;
        };
        SafeMap<uintptr_t, tagItem> m_sockObjectMap;
    public:
        Poll();
        ~Poll() override;

        static apr_pollset_t *createPollSet(apr_pool_t *pool, uint32_t size);

        int add(const std::shared_ptr<PollEventBase> &pObject, const std::shared_ptr<NetWorker> &net);

        // 所有权转移, 必须是PollEventBase子类
        template<typename T, typename ...Args>
        int add(const std::shared_ptr<NetWorker> &net, Args ...args) {
            auto deleter = [this](PollEventBase *p) {
                this->remove(p);
                delete p;
            };
            auto event = std::shared_ptr<PollEventBase>(new T(net, std::forward<Args>(args)...), deleter);
            return add(event, net);
        }

        int remove(PollEventBase *event, const std::string &op = "");

        template<typename T,
                typename std::enable_if<std::is_base_of<PollEventBase, T>::value, int>::type = 0>
        std::shared_ptr<T> find() const {
            Sync(m_sockObjectMap) {
               for (const auto &it: m_sockObjectMap) {
                   auto ret = std::dynamic_pointer_cast<T>(it.second.event);
                   if (ret) {
                       return ret;
                   }
               }
            }}}
            return nullptr;
        }
    private:
        void doRun() override;
        int remove(uintptr_t c, const std::string &op);
        int removeFromPoll(const tagItem &item, uintptr_t, const std::string &);
        int createPollSet();
        uintptr_t find(PollEventBase *event) const;
    };

    typedef Singleton<Poll> SingletonPoll;
}

#ifdef ENABLE_COVERAGE
void StartGlobalPoll();
#endif

#endif
