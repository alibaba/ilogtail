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
    mSendInterval = 60;
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
    /*
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
    */
    std::unordered_map<std::string, PipelineMetric*> pipeLineMetrics = ILogtailMetric::GetInstance()->mPipelineMetrics;
    for (std::unordered_map<std::string, PipelineMetric*>::iterator iter = pipeLineMetrics.begin(); iter != pipeLineMetrics.end(); ++iter) {
        LOG_INFO(sLogger, ("pipeline_metric value", iter->first));
    }
    mLastSendTime = curTime;
}

void MetricExportor::pushPluginMetric(bool forceSend) {

}

void MetricExportor::pushSubPluginMetric(bool forceSend) {

}

}