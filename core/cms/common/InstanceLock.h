#ifndef ARGUS_COMMON_INSTANCE_LOCK_H
#define ARGUS_COMMON_INSTANCE_LOCK_H

#include <mutex>

typedef std::recursive_mutex InstanceLock;
typedef std::lock_guard<std::recursive_mutex> LockGuard;

namespace common {
    using ::InstanceLock;
    using ::LockGuard;
}

#endif // ARGUS_COMMON_INSTANCE_LOCK_H
