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

    // 根据config参数构造基础信息
    for (auto item : config["PrometheusScrapeConfig"]) {
        ScrapeJob job(item);
        // 过渡方法 传参targets
        if (item["scrapeTargets"].isArray()) {
            for (auto targetConfig : item["scrapeTargets"]) {
                ScrapeTarget target(targetConfig);
                target.jobName = job.jobName;
                target.scheme = job.scheme;
                target.metricsPath = job.metricsPath;
                target.scrapeInterval = job.scrapeInterval;
                target.scrapeTimeout = job.scrapeTimeout;
                target.targetId = job.jobName + "-index-" + to_string(job.scrapeTargets.size());
                job.scrapeTargets.push_back(target);
            }
        }
        scrapeJobs.push_back(job);
    }

    // 从Master中请求scrapetargets，应当放在start中，但当前缺少依赖master，由config传入

    // 为每个job设置queueKey、inputIndex
    for (ScrapeJob& job : scrapeJobs) {
        job.queueKey = mContext->GetProcessQueueKey();
        job.inputIndex = 0;
        for (ScrapeTarget& target : job.scrapeTargets) {
            target.jobName = job.jobName;
            target.queueKey = mContext->GetProcessQueueKey();
            target.inputIndex = 0;
        }
    }

    return true;
}

// 更新targets
bool InputPrometheus::Start() {
    PrometheusInputRunner::GetInstance()->UpdateScrapeInput(mContext->GetConfigName(), scrapeJobs);
    return true;
}

bool InputPrometheus::Stop(bool isPipelineRemoving) {
    PrometheusInputRunner::GetInstance()->RemoveScrapeInput(mContext->GetConfigName());
    return true;
}

} // namespace logtail