// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "monitor/MetricManager.h"
#include "unittest/Unittest.h"
#include "provider/Provider.h"


namespace logtail {

class ProviderUnittest : public testing::Test {
public:
    void TestGetRemoteConfigProvider();
    void TestGetProfileSender();
    void TestGetReadMetrics();

};
void ProviderUnittest::TestGetRemoteConfigProvider() {
    auto remoteConfigProviders = GetRemoteConfigProviders();
    APSARA_TEST_GT(remoteConfigProviders.size(), 0U);
}

void ProviderUnittest::TestGetReadMetrics() {
    auto readMetrics = GetReadMetrics();
    auto expected = ReadMetrics::GetInstance();
    APSARA_TEST_NOT_EQUAL(nullptr, readMetrics);
    APSARA_TEST_EQUAL(expected, readMetrics);
}

void ProviderUnittest::TestGetProfileSender() {
    auto profileSender = GetProfileSender();
    APSARA_TEST_NOT_EQUAL(nullptr, profileSender);
}


UNIT_TEST_CASE(ProviderUnittest, TestGetRemoteConfigProvider)
UNIT_TEST_CASE(ProviderUnittest, TestGetProfileSender)
UNIT_TEST_CASE(ProviderUnittest, TestGetReadMetrics)
} // namespace logtail

UNIT_TEST_MAIN