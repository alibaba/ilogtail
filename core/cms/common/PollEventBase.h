#ifndef _POLL_EVENT_BASE_H_
#define _POLL_EVENT_BASE_H_

#include <memory>

namespace common {
    class NetWorker;

    class PollEventBase {
    public:
        virtual ~PollEventBase() = default;
        virtual int readEvent(const std::shared_ptr<PollEventBase> &myself) = 0;
    };
}

#endif
