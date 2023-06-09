#include "MetricExportor.h"
#include "sender/Sender.h"
#include "log_pb/sls_logs.pb.h"
#include "common/LogtailCommonFlags.h"

using namespace sls_logs;
using namespace std;

namespace logtail {

MetricExportor::MetricExportor() {
    mSendInterval = 10;
    mSnapshotInterval = 10;
    mLastSendTime = 0;
    mLastSnapshotTime = 0;
}

void MetricExportor::pushMetrics() {
    pushInstanceMetric(false);
    pushPluginMetric(false);
    pushSubPluginMetric(false);
}


void MetricExportor::snapshotMetrics(bool force) {
    int32_t curTime = time(NULL);
    if (!force && (curTime - mLastSnapshotTime < mSnapshotInterval)) {
        return;
    }
    mSnapshotPipelineMetrics.clear();
    std::list<PipelineMetric*> pipeLineMetrics = ILogtailMetric::GetInstance()->mPipelineMetrics;
    for (std::list<PipelineMetric*>::iterator iter = pipeLineMetrics.begin(); iter != pipeLineMetrics.end(); ++iter) {
        //LOG_INFO(sLogger, ("pipeline_metric key", iter->first));
        PipelineMetric* newPilelineMetric = new PipelineMetric();
        PipelineMetric* pilelineMetric = *iter;
        for (std::unordered_map<std::string, BaseMetric*>::iterator iterMetric = pilelineMetric->mBaseMetrics.begin(); iterMetric != pilelineMetric->mBaseMetrics.end(); ++ iterMetric) {
            BaseMetric* newBaseMetric = new BaseMetric(iterMetric->second->snapShotMetricObj());
            newPilelineMetric->mBaseMetrics.insert(std::pair<std::string, BaseMetric*>(iterMetric->first, newBaseMetric));
            long value = newBaseMetric->getMetricObj()->val;
            LOG_INFO(sLogger, ("base_metric key", iterMetric->first));
            LOG_INFO(sLogger, ("base_metric val", value));
        }

        for (std::unordered_map<std::string, std::string>::iterator iterLabel = pilelineMetric->mLabels.begin(); iterLabel != pilelineMetric->mLabels.end(); ++ iterLabel) {
            LOG_INFO(sLogger, ("label key", iterLabel->first));
            LOG_INFO(sLogger, ("label value", iterLabel->second));
            newPilelineMetric->mLabels.insert(std::pair<std::string, std::string>(iterLabel->first, iterLabel->second));
        }
        mSnapshotPipelineMetrics.push_back(newPilelineMetric);
    }
    snapshotPluginMetrics();
}

void MetricExportor::snapshotPluginMetrics() {
    PluginPipelineMetric inner = LogtailPlugin::GetInstance()->GetPipelineMetrics("test");
    LOG_INFO(sLogger, ("innerPipelineMetric key", inner.pipelineName));
    LOG_INFO(sLogger, ("innerPipelineMetric value", inner.value));
}


void MetricExportor::pushInstanceMetric(bool forceSend) {
    int32_t curTime = time(NULL);
    if (!forceSend && (curTime - mLastSendTime < mSendInterval)) {
        return;
    }
    
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
   snapshotMetrics(true);

    for (std::list<PipelineMetric*>::iterator iter = mSnapshotPipelineMetrics.begin(); iter != mSnapshotPipelineMetrics.end(); ++iter) {
        //LOG_INFO(sLogger, ("pipeline_metric key", iter->first));
        PipelineMetric* pilelineMetric = *iter;
        for ( std::unordered_map<std::string, BaseMetric*>::iterator iterMetric = pilelineMetric->mBaseMetrics.begin(); iterMetric != pilelineMetric->mBaseMetrics.end(); ++ iterMetric) {
            LOG_INFO(sLogger, ("base_metric key", iterMetric->first));
            long value = iterMetric->second->getMetricObj()->val;

            LOG_INFO(sLogger, ("base_metric val", value));
        }
    }
    mLastSendTime = curTime;
}

void MetricExportor::pushPluginMetric(bool forceSend) {

}

void MetricExportor::pushSubPluginMetric(bool forceSend) {

}

}