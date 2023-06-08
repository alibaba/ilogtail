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


    std::list<PipelineMetric*> pipeLineMetrics = ILogtailMetric::GetInstance()->mPipelineMetrics;

    for (std::list<PipelineMetric*>::iterator iter = pipeLineMetrics.begin(); iter != pipeLineMetrics.end(); ++iter) {
        //LOG_INFO(sLogger, ("pipeline_metric key", iter->first));
        PipelineMetric* pilelineMetric = *iter;
        for ( std::unordered_map<std::string, BaseMetric*>::iterator it4 = pilelineMetric->mBaseMetrics.begin(); it4 != pilelineMetric->mBaseMetrics.end(); ++ it4) {
            LOG_INFO(sLogger, ("base_metric key", it4->first));
            LOG_INFO(sLogger, ("base_metric val", it4->second->getMetricObj()->val));
        }
    }
    mLastSendTime = curTime;
}

void MetricExportor::pushPluginMetric(bool forceSend) {

}

void MetricExportor::pushSubPluginMetric(bool forceSend) {

}

}