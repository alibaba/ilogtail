#pragma once

#include <cstdint>

namespace logtail {

class MetricExportor {
public:
    static MetricExportor* GetInstance() {
        static MetricExportor* ptr = new MetricExportor();
        return ptr;
    }
    void PushMetrics(bool forceSend);

private:
    MetricExportor();
    int32_t mSendInterval;
    int32_t mLastSendTime;
};
}