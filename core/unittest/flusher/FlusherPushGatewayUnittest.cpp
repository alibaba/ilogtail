// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <thread>

#include "sdk/CurlAsynInstance.h"
#include "plugin/instance/FlusherInstance.h"
#include "flusher/FlusherPushGateway.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class FlusherPushGatewayUnittest : public testing::Test {
public:
    void SimpleTest();

protected:
    void SetUp() override { ctx.SetConfigName("test_config"); }

private:
    PipelineContext ctx;
};

void FlusherPushGatewayUnittest::SimpleTest() {
    FlusherPushGateway* flusher = new FlusherPushGateway();
    unique_ptr<FlusherInstance> flusherIns = unique_ptr<FlusherInstance>(new FlusherInstance(flusher, "0"));
    Json::Value config, goPipeline;

    config["pushGatewayScheme"] = "https";
    config["pushGatewayHost"] = "cn-hangzhou.arms.aliyuncs.com";
    config["pushGatewayPath"] = "/prometheus/<toke >/<userId>/<clusterId>/cn-hangzhou/api/v2/metrics/job/push_gateway_job";

    flusherIns->Init(config, ctx, goPipeline);
    TestSender* sender = new TestSender;
    flusher->mItemSender.reset(sender);
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

    APSARA_TEST_EQUAL(1UL, sender->mItems.size());

    auto item = sender->mItems.at(0);
    auto req = flusher->BuildRequest(item);
    auto curlIns = sdk::CurlAsynInstance::GetInstance();
    curlIns->AddRequest(req);
    PromiseClosure* promiseClosure = (PromiseClosure*)req->mCallBack;
    auto future = promiseClosure->mPromise.get_future();
    auto resInfo = future.get();
    APSARA_TEST_STREQ("", req->mBody.data());
    APSARA_TEST_STREQ("", resInfo.errorCode.data());
    APSARA_TEST_STREQ("", resInfo.errorMessage.data());
    APSARA_TEST_EQUAL(200, resInfo.response->statusCode);
}

UNIT_TEST_CASE(FlusherPushGatewayUnittest, SimpleTest)

} // namespace logtail

UNIT_TEST_MAIN
