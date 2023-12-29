#pragma once

#include "profile_sender/ProfileSender.h"


namespace logtail {

class MetricExportor {
public:
    static MetricExportor* GetInstance() {
        static MetricExportor* ptr = new MetricExportor();
        return ptr;
    }
    void PushMetrics(bool forceSend);
    void PushGoPluginMetrics();
    void SendMetrics(std::map<std::string, sls_logs::LogGroup*>& logGroupMap);

private:
    MetricExportor();
    ProfileSender mProfileSender;
    int32_t mSendInterval;
    int32_t mLastSendTime;
};
}