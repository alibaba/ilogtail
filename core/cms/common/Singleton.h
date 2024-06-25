#ifndef ARGUS_COMMON_SINGLETON_H
#define ARGUS_COMMON_SINGLETON_H

#include <mutex>
#include <type_traits>
#include <functional>

#include "InstanceLock.h"

void registerDestroy(int flag, const std::function<void()> &fnDestroy);
// 删除所有的singleton实例
void destroySingletons(int flag);

#define SINGLE_FLAG_PRODUCT 0

template<class T, int = SINGLE_FLAG_PRODUCT>
class Singleton {
public:
    typedef T Type;

    static T *Instance();
    static void destroyInstance();

    static T *swap(T *newT) {
        std::lock_guard<InstanceLock> _guard(m_locker);
        T *old = pInstance;
        pInstance = newT;
        return old;
    }

    template<typename TChild>
    static typename std::enable_if<std::is_base_of<T, TChild>::value, TChild *>::type cast() {
        std::lock_guard<InstanceLock> _guard(m_locker);
        return dynamic_cast<TChild *>(Instance());
    }

private:
    static T *pInstance;
    static InstanceLock m_locker;
};

template<class T, int Flag>
inline T *Singleton<T, Flag>::Instance() {
    if (pInstance == nullptr) {
        std::lock_guard<InstanceLock> _guard(m_locker);
        if (pInstance == nullptr) {
            pInstance = new T;
            if (pInstance) {
                registerDestroy(Flag, &Singleton<T, Flag>::destroyInstance);
            }
        }
    }
    return pInstance;
}

template<class T, int Flag>
void Singleton<T, Flag>::destroyInstance() {
    if (pInstance != nullptr) {
        std::lock_guard<InstanceLock> _guard(m_locker);
        if (pInstance) {
            delete pInstance;
            pInstance = nullptr;
        }
    }
}

template<class T, int Flag>
InstanceLock Singleton<T, Flag>::m_locker;

template<class T, int Flag>
T *Singleton<T, Flag>::pInstance = nullptr;

namespace common {
    using ::Singleton;
}

#endif // ARGUS_COMMON_SINGLETON_H
