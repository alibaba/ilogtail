#include <cstdlib>

#include "flusher/FlusherRemoteWrite.h"
#include "plugin/instance/FlusherInstance.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {
class FlusherRemoteWriteTest : public testing::Test {
public:
    void SimpleTest();

protected:
    void SetUp() override { ctx.SetConfigName("test_config"); }

private:
    PipelineContext ctx;
};

void FlusherRemoteWriteTest::SimpleTest() {
    FlusherRemoteWrite* flusher = new FlusherRemoteWrite();
    unique_ptr<FlusherInstance> flusherIns = unique_ptr<FlusherInstance>(new FlusherInstance(flusher, "0"));
    Json::Value config, goPipeline;
    // {
    //   "Type": "flusher_remote_write/flusher_push_gateway",
    //   "Endpoint": "cn-hangzhou.arms.aliyuncs.com",
    //   "Scheme": "https",
    //   "UserId": "******",
    //   "ClusterId": "*******",
    //   "Region": "cn-hangzhou"
    // }
    config["endpoint"] = string(getenv("ENDPOINT"));
    config["scheme"] = "https";
    config["user_id"] = string(getenv("USER_ID"));
    config["cluster_id"] = string(getenv("CLUSTER_ID"));
    config["region"] = string(getenv("REGION"));

    flusherIns->Init(config, ctx, goPipeline);
    const auto& srcBuf = make_shared<SourceBuffer>();
    auto eGroup = PipelineEventGroup(srcBuf);
    auto event = eGroup.AddMetricEvent();
    event->SetName("test_metric");
    event->SetValue(MetricValue(UntypedSingleValue{1.0}));
    event->SetTimestamp(1719414245);
    event->SetTag(StringView("test_key_x"), StringView("test_value_x"));

    flusherIns->Start();
    flusherIns->Send(std::move(eGroup));
    flusherIns->FlushAll();
}

} // namespace logtail