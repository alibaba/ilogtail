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
    //pushPluginMetric(false);
    //pushSubPluginMetric(false);
}


void MetricExportor::snapshotMetrics(bool force) {
    int32_t curTime = time(NULL);
    if (!force && (curTime - mLastSnapshotTime < mSnapshotInterval)) {
        return;
    }
    mSnapshotPipelineMetrics.clear();
    std::lock_guard<std::mutex> lock(ILogtailMetric::GetInstance()->mMetricsLock);
    std::list<PipelineMetricPtr> pipeLineMetrics = ILogtailMetric::GetInstance()->mPipelineMetrics;
    for (std::list<PipelineMetricPtr>::iterator iter = pipeLineMetrics.begin(); iter != pipeLineMetrics.end(); ++iter) {
        //LOG_INFO(sLogger, ("pipeline_metric key", iter->first));
        PipelineMetricPtr newPilelineMetric(new PipelineMetric());
        PipelineMetricPtr pilelineMetric = *iter;
        for (std::unordered_map<std::string, BaseMetricPtr>::iterator iterMetric = pilelineMetric->mBaseMetrics.begin(); iterMetric != pilelineMetric->mBaseMetrics.end(); ++ iterMetric) {
            BaseMetricPtr newBaseMetric(new BaseMetric(iterMetric->second->snapShotMetricObj()));
            newPilelineMetric->mBaseMetrics.insert(std::pair<std::string, BaseMetricPtr>(iterMetric->first, newBaseMetric));
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
    sls_logs::LogGroup logGroup;
    logGroup.set_category("metric-test");
    // logGroup.set_source(LogFileProfiler::mIpAddr);
    std::string region = "";
    LOG_INFO(sLogger, ("mSnapshotPipelineMetrics size", mSnapshotPipelineMetrics.size()));

    for (std::list<PipelineMetricPtr>::iterator iter = mSnapshotPipelineMetrics.begin(); iter != mSnapshotPipelineMetrics.end(); ++iter) {
        //LOG_INFO(sLogger, ("pipeline_metric key", iter->first));
        PipelineMetricPtr pilelineMetric = *iter;
        for ( std::unordered_map<std::string, BaseMetricPtr>::iterator iterMetric = pilelineMetric->mBaseMetrics.begin(); iterMetric != pilelineMetric->mBaseMetrics.end(); ++ iterMetric) {
            LOG_INFO(sLogger, ("base_metric key", iterMetric->first));
            long value = iterMetric->second->getMetricObj()->val;
            BuildLogFromMetric(logGroup, pilelineMetric);
            LOG_INFO(sLogger, ("base_metric val", value));
        }
    }
    mProfileSender.SendMetric(logGroup);
    mLastSendTime = curTime;
}


void MetricExportor::BuildLogFromMetric(sls_logs::LogGroup& logGroup, PipelineMetricPtr pipelineMetric) {
    std::string labelStr = BuildMetricLabel(pipelineMetric->mLabels);
    for (std::unordered_map<std::string, BaseMetricPtr>::iterator iterMetric = pipelineMetric->mBaseMetrics.begin(); iterMetric != pipelineMetric->mBaseMetrics.end(); ++ iterMetric) {
        Log* logPtr = logGroup.add_logs();
        logPtr->set_time(time(NULL));
        Log_Content* contentPtr = logPtr->add_contents();
        contentPtr->set_key("__name__");
        contentPtr->set_value(iterMetric->first);
        contentPtr = logPtr->add_contents();
        contentPtr->set_key("__value__");
        contentPtr->set_value(std::to_string(iterMetric->second->mMetricObj->val));
        contentPtr = logPtr->add_contents();
        contentPtr->set_key("__time_nano__");
        contentPtr->set_value(std::to_string(iterMetric->second->mMetricObj->timestamp) + "000");
        contentPtr = logPtr->add_contents();
        contentPtr->set_key("__labels__");
        contentPtr->set_value(labelStr);
    }
}

std::string MetricExportor::BuildMetricLabel(std::unordered_map<std::string, std::string> labels) {
    std::string labelStr = "";
    int count = 0;
    for (std::unordered_map<std::string, std::string>::iterator iterLabel = labels.begin(); iterLabel != labels.end(); ++ iterLabel) {
        LOG_INFO(sLogger, ("label key", iterLabel->first));
        LOG_INFO(sLogger, ("label value", iterLabel->second));
        labelStr += iterLabel->first;
        labelStr += "#$#";
        labelStr += iterLabel->second;
        if (count < labels.size() -1) {
            labelStr += "|";
        }
        count ++;
    }
    return labelStr;
}


}