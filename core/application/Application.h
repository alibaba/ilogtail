#pragma once

#include <cstdint>
#include <string>

#include "common/Thread.h"
#include "common/Lock.h"

namespace logtail {

class Application {
public:
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    static Application* GetInstance() {
        static Application instance;
        return &instance;
    }

    void Start();

    int32_t GetStartTime() { return mStartTime; }
    std::string GetInstanceId() { return mInstanceId; }
    bool TryGetUUID();
    std::string GetUUID() {
        mUUIDLock.lock();
        std::string uuid(mUUID);
        mUUIDLock.unlock();
        return uuid;
    }

private:
    Application();
    ~Application();

    void Exit();
    void CheckCriticalCondition(int32_t curTime);

    bool GetUUIDThread();
    void SetUUID(std::string uuid) {
        mUUIDLock.lock();
        mUUID = uuid;
        mUUIDLock.unlock();
        return;
    }

    std::string mInstanceId;
    int32_t mStartTime;
    ThreadPtr mUUIDthreadPtr;
    SpinLock mUUIDLock;
    std::string mUUID;
};

} // namespace logtail
