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


void MetricExportor::BuildLogFromMetric(LogGroup& logGroup, PipelineMetric* pipelineMetric) {
    std::string labelStr = BuildMetricLabel(pipelineMetric->mLabels);
    for (std::unordered_map<std::string, BaseMetric*>::iterator iterMetric = pilelineMetric->mBaseMetrics.begin(); iterMetric != pilelineMetric->mBaseMetrics.end(); ++ iterMetric) {
        Log* logPtr = logGroup.add_logs();
        logPtr->set_time(time(NULL));
        Log_Content* contentPtr = logPtr->add_contents();
        contentPtr->set_key("__name__");
        contentPtr->set_value(iterMetric->first);
        contentPtr = logPtr->add_contents();
        contentPtr->set_key("__value__");
        contentPtr->set_value(iterMetric->second->val);
        contentPtr = logPtr->add_contents();
        contentPtr->set_key("__time_nano__");
        contentPtr->set_value(iterMetric->second->timestamp);
        contentPtr = logPtr->add_contents();
        contentPtr->set_key("__labels__");
        contentPtr->set_value(statistic->mFilename.empty() ? "logstore_statistics" : statistic->mFilename);
    }
}

std::string BuildMetricLabel(std::unordered_map<std::string, std::string> labels) {
    std::string labelStr = "|";
    for (std::unordered_map<std::string, std::string>::iterator iterLabel = labels.begin(); iterLabel != labels.end(); ++ iterLabel) {
        LOG_INFO(sLogger, ("label key", iterLabel->first));
        LOG_INFO(sLogger, ("label value", iterLabel->second));
        labelStr += iterLabel->first;
        labelStr += "#$#";
        labelStr += iterLabel->second;
    }
    return buildMetricLabel;
}


}