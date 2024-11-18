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

#include "CollectorManager.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class CollectorManagerUnittest : public testing::Test {
public:
    void TestGetCollector() const;
};

void CollectorManagerUnittest::TestGetCollector() const {
    {
        auto collector1 = CollectorManager::GetInstance()->GetCollector("mock");
        APSARA_TEST_NOT_EQUAL_FATAL(nullptr, collector1);
        APSARA_TEST_EQUAL_FATAL("mock", collector1->GetName());
        auto collector2 = CollectorManager::GetInstance()->GetCollector("mock");
        APSARA_TEST_EQUAL_FATAL(collector1, collector2);
    }
}


UNIT_TEST_CASE(CollectorManagerUnittest, TestGetCollector);

} // namespace logtail

UNIT_TEST_MAIN
