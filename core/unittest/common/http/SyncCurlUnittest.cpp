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

#include "common/http/HttpRequest.h"
#include "common/http/HttpResponse.h"
#include "common/http/SyncCurl.h"
#include "unittest/Unittest.h"


using namespace std;

namespace logtail {

class SyncCurlUnittest : public ::testing::Test {
public:
    void TestSend();
};


void SyncCurlUnittest::TestSend() {
    std::unique_ptr<HttpRequest> request;
    HttpResponse res;
    request = std::make_unique<HttpRequest>("GET", false, "example.com", 80, "/path", "", map<string, string>(), "", 10, 3);
    bool success = Send(std::move(request), res);
    APSARA_TEST_TRUE(success);
    APSARA_TEST_EQUAL(404, res.mStatusCode);
}

UNIT_TEST_CASE(SyncCurlUnittest, TestSend)

} // namespace logtail

UNIT_TEST_MAIN