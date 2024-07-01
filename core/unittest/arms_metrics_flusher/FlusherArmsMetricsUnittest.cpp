
#include <json/json.h>

#include <memory>
#include <string>

#include "common/JsonUtil.h"
#include "common/LogstoreFeedbackKey.h"
#include "common/LogtailCommonFlags.h"
#ifdef __ENTERPRISE__
#include "config/provider/EnterpriseConfigProvider.h"
#endif
#include "flusher/FlusherArmsMetrics.h"
#include "pipeline/PipelineContext.h"
#include "sender/SLSClientManager.h"
#include "sender/Sender.h"
#include "unittest/Unittest.h"

DECLARE_FLAG_INT32(batch_send_interval);
DECLARE_FLAG_INT32(merge_log_count_limit);
DECLARE_FLAG_INT32(batch_send_metric_size);

using namespace std;

namespace logtail {

class FlusherArmsMetricsUnittest : public testing::Test {
public:
    void OnSuccessfulInit();
    void Send();

protected:
    void SetUp() override {
        ctx.SetConfigName("test_config");
        mSourceBuffer.reset(new SourceBuffer);
        mEventGroup.reset(new PipelineEventGroup(mSourceBuffer));
    }

private:
    PipelineContext ctx;
    std::shared_ptr<SourceBuffer> mSourceBuffer;

    std::unique_ptr<PipelineEventGroup> mEventGroup;
};

void FlusherArmsMetricsUnittest::Send() {
}

void FlusherArmsMetricsUnittest::OnSuccessfulInit() {
    unique_ptr<FlusherArmsMetrics> flusher;
    Json::Value configJson, optionalGoPipelineJson, optionalGoPipeline;
    string configStr, optionalGoPipelineStr, errorMsg;

    // only mandatory param
    configStr = R"(
        {
            "Type": "flusher_arms_metrics",
            "Project": "test_project",
            "Logstore": "test_logstore",
            "Region": "cn-hk",
            "Licensekey": "test-licenseKey",
            "PushAppId": "test-push-appId",
            "Endpoint": "cn-hangzhou.log.aliyuncs.com"
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    flusher.reset(new FlusherArmsMetrics());
    flusher->SetContext(ctx);
    flusher->Init(configJson, optionalGoPipeline);
    LOG_INFO(sLogger, ("AddMetricEvent", "start to add metricsEvent"));
    LOG_INFO(sLogger, ("Set Metrics Name", "start to  Set Metrics name"));
    mEventGroup->SetTag(std::string("workloadName"), std::string("arms-oneagent-test"));
    mEventGroup->SetTag(std::string("workloadKind"), std::string("Deployment"));
    mEventGroup->SetTag(std::string("pid"), std::string("1d15aab965811c49f42019136da3958b"));
    mEventGroup->SetTag(std::string("appId"), std::string("1d15aab965811c49f42019136da3958b"));
    mEventGroup->SetTag(std::string("source_ip"), std::string("10.54.0.33"));
    mEventGroup->SetTag(std::string("host"), std::string("10.54.0.33"));
    mEventGroup->SetTag(std::string("rpc"), std::string("/oneagent/lurious/local"));
    mEventGroup->SetTag(std::string("rpcType"), std::string("0"));
    mEventGroup->SetTag(std::string("callType"), std::string("http"));
    mEventGroup->SetTag(std::string("appType"), std::string("EBPF"));
    mEventGroup->SetTag(std::string("statusCode"), std::string("200"));
    mEventGroup->SetTag(std::string("version"), std::string("HTTP1.1"));
    mEventGroup->SetTag(std::string("source"), std::string("ebpf"));
    for (int i = 100 - 1; i >= 0; i--) {
        auto metricsEvent = mEventGroup->AddMetricEvent();
        metricsEvent->SetName("arms_rpc_requests_count");
        metricsEvent->SetValue(UntypedSingleValue{120.0});
    }

    LOG_INFO(sLogger, ("OnSuccessfulInit", "start to sender"));
    flusher->Send(std::move(*mEventGroup));
    APSARA_TEST_EQUAL("", flusher->mAliuid);
}


UNIT_TEST_CASE(FlusherArmsMetricsUnittest, OnSuccessfulInit)
} // namespace logtail

int main(int argc, char** argv) {
    InitUnittestMain();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
