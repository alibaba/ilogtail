#include "input/InputPrometheus.h"

#include "common/ParamExtractor.h"
#include "pipeline/PipelineContext.h"
#include "prometheus/PrometheusInputRunner.h"

using namespace std;

namespace logtail {

const string InputPrometheus::sName = "input_prometheus";

InputPrometheus::InputPrometheus() {
}

bool InputPrometheus::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    string errorMsg;

    // 服务发现

    // 根据config参数构造ScrapeJobs

    // 为每个job设置queueKey、inputIndex
    for (ScrapeJob& job : scrapeJobs) {
        job.queueKey = mContext->GetProcessQueueKey();
        job.inputIndex = 0;
        for (ScrapeTarget& target : job.scrapeTargets) {
            target.queueKey = mContext->GetProcessQueueKey();
            target.inputIndex = 0;
        }
    }

    return true;
}

// 更新targets等
bool InputPrometheus::Start() {
    PrometheusInputRunner::GetInstance()->UpdateScrapeInput(mContext->GetConfigName(), scrapeJobs);
    return true;
}

bool InputPrometheus::Stop(bool isPipelineRemoving) {
    // 用清理暂时代替回收
    // PrometheusInputRunner::GetInstance()->targets.clear();
    return true;
}

} // namespace logtail