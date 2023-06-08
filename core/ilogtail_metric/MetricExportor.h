#pragma once

#include <string>
#include <mutex>
#include <unordered_map>
#include <map>
#include <json/json.h>
#include "profile_sender/ProfileSender.h"
#include "ILogtailMetric.h"
#include "logger/Logger.h"

namespace logtail {

class MetricExportor {
public:
    static MetricExportor* GetInstance() {
        static MetricExportor* ptr = new MetricExportor();
        return ptr;
    }
    void pushMetrics();
    void pullMetrics();


private:
    MetricExportor();
    ~MetricExportor() {}

    

    void pushInstanceMetric(bool forceSend);
    void pushPluginMetric(bool forceSend);
    void pushSubPluginMetric(bool forceSend);

    ProfileSender mProfileSender;
    int32_t mSendInterval;
    int32_t mLastSendTime;
};
}