#include "MetricExportor.h"
#include <string>
#include "sender/Sender.h"
#include "ILogtailMetric.h"
#include "log_pb/sls_logs.pb.h"
#include "common/LogtailCommonFlags.h"

using namespace sls_logs;
using namespace std;

namespace logtail {

MetricExportor::MetricExportor() {
    mSendInterval = INT32_FLAG(profile_data_send_interval);
}

void MetricExportor::pushMetrics() {
    pushInstanceMetric(false);
    pushPluginMetric(false);
    pushSubPluginMetric(false);
}

void MetricExportor::pushInstanceMetric(bool forceSend) {
    int32_t curTime = time(NULL);

    if (!forceSend && (curTime - mLastSendTime < mSendInterval))
        return;
    
    size_t sendRegionIndex = 0;
    Json::Value detail;
    Json::Value logstore;
    do {
        LogGroup logGroup;
        logGroup.set_category("shennong_log_profile");
        string region;
        
        mProfileSender.SendToProfileProject(region, logGroup);
        break;
    } while (true);
    mLastSendTime = curTime;
}

void MetricExportor::pushPluginMetric(bool forceSend) {

}

void MetricExportor::pushSubPluginMetric(bool forceSend) {

}

}