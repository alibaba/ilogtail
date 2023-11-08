#include "application/Application.h"

#include <thread>

#include "common/StringTools.h"
#include "common/util.h"
#include "monitor/LogFileProfiler.h"

using namespace std;

namespace logtail {

Application::Application() : mStartTime(time(nullptr)) {
    mInstanceId = CalculateRandomUUID() + "_" + LogFileProfiler::mIpAddr + "_" + ToString(time(NULL));
}

Application::~Application() {
    try {
        if (mUUIDthreadPtr.get() != NULL)
            mUUIDthreadPtr->GetValue(100);
    } catch (...) {
    }
}

bool Application::TryGetUUID() {
    mUUIDthreadPtr = CreateThread([this]() { GetUUIDThread(); });
    // wait 1000 ms
    for (int i = 0; i < 100; ++i) {
        this_thread::sleep_for(chrono::milliseconds(10));
        if (!GetUUID().empty()) {
            return true;
        }
    }
    return false;
}

bool Application::GetUUIDThread() {
    string uuid;
#if defined(__aarch64__) || defined(__sw_64__)
    // DMI can not work on such platforms but might crash Logtail, disable.
#else
    uuid = CalculateDmiUUID();
#endif
    SetUUID(uuid);
    return true;
}

} // namespace logtail
