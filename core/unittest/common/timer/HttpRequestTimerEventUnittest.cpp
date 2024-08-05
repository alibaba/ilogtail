// Copyright 2024 iLogtail Authors
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

#include "common/http/AsynCurlRunner.h"
#include "common/timer/HttpRequestTimerEvent.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

struct AsynHttpRequestMock : public AsynHttpRequest {
    AsynHttpRequestMock(const string& method,
                        bool httpsFlag,
                        const string& host,
                        const string& url,
                        const string& query,
                        const map<string, string>& header,
                        const string& body)
        : AsynHttpRequest(method, httpsFlag, host, url, query, header, body) {}

    bool IsContextValid() const override { return true; };
    void OnSendDone(const HttpResponse& response) {};
};

class HttpRequestTimerEventUnittest : public ::testing::Test {
public:
    void TestValid();
    void TestExecute();
};

void HttpRequestTimerEventUnittest::TestValid() {
    HttpRequestTimerEvent event(chrono::steady_clock::now(),
                                make_unique<AsynHttpRequestMock>("", false, "", "", "", map<string, string>(), ""));
    APSARA_TEST_TRUE(event.IsValid());
}

void HttpRequestTimerEventUnittest::TestExecute() {
    HttpRequestTimerEvent event(chrono::steady_clock::now(),
                                make_unique<AsynHttpRequestMock>("", false, "", "", "", map<string, string>(), ""));
    event.Execute();
    APSARA_TEST_EQUAL(1U, AsynCurlRunner::GetInstance()->mQueue.Size());
}

UNIT_TEST_CASE(HttpRequestTimerEventUnittest, TestValid)
UNIT_TEST_CASE(HttpRequestTimerEventUnittest, TestExecute)

} // namespace logtail

UNIT_TEST_MAIN
