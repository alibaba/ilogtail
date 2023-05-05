#pragma once

#include <string>
#include <mutex>
#include <unordered_map>
#include <map>
#include <json/json.h>
#include "profile_sender/ProfileSender.h"

namespace logtail {

class MetricExportor {
public:
    void pushMetrics();
    void pullMetrics();


private:
    MetricExportor();
    ~MetricExportor() {}
    void getILogtailInstanceMetrics();
    void getPluginMetrics();
    void getSubPluginMetrics();

    void pushInstanceMetric(bool forceSend);
    void pushPluginMetric(bool forceSend);
    void pushSubPluginMetric(bool forceSend);

    ProfileSender mProfileSender;
    int32_t mSendInterval;
    int32_t mLastSendTime;
};
}