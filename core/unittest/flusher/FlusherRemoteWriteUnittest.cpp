#include <cstdlib>

#include "flusher/FlusherRemoteWrite.h"
#include "plugin/instance/FlusherInstance.h"
#include "sdk/CurlAsynInstance.h"
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
    config["Endpoint"] = string(getenv("ENDPOINT"));
    config["Scheme"] = "https";
    config["UserId"] = string(getenv("USER_ID"));
    config["ClusterId"] = string(getenv("CLUSTER_ID"));
    config["Region"] = string(getenv("REGION"));

    flusherIns->Init(config, ctx, goPipeline);
    const auto& srcBuf = make_shared<SourceBuffer>();
    auto eGroup = PipelineEventGroup(srcBuf);
    auto event = eGroup.AddMetricEvent();
    event->SetName("test_metric");
    event->SetValue(MetricValue(UntypedSingleValue{1.0}));
    event->SetTimestamp(1719414245);
    event->SetTag(StringView("test_key_x"), StringView("test_value_x"));
    event->SetTag(StringView("__job__"), StringView("remote_write_job"));
    event->SetTag(StringView("__name__"), StringView("test_metric"));

    flusherIns->Start();
    flusherIns->Send(std::move(eGroup));
    flusherIns->FlushAll();

    APSARA_TEST_EQUAL(1UL, flusher->mItems.size());

    auto item = flusher->mItems.at(0);
    auto req = flusher->BuildRequest(item);
    auto curlIns = sdk::CurlAsynInstance::GetInstance();
    curlIns->AddRequest(req);
    RemoteWriteClosure* closure = (RemoteWriteClosure*)req->mCallBack;
    auto future = closure->mPromise.get_future();
    auto resInfo = future.get();
    // APSARA_TEST_STREQ("", req->mBody.data());
    APSARA_TEST_STREQ("", resInfo.errorCode.data());
    APSARA_TEST_STREQ("", resInfo.errorMessage.data());
    APSARA_TEST_EQUAL(200, resInfo.statusCode);
    delete item;
}

UNIT_TEST_CASE(FlusherRemoteWriteTest, SimpleTest)

} // namespace logtail

UNIT_TEST_MAIN
