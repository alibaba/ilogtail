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

#include <memory>
#include <string>
#include "unittest/Unittest.h"
#include "cms/CmsMain.h"

using namespace std;

namespace logtail {

class CmsMainUnittest : public testing::Test {
public:
    void OnName() {
        APSARA_TEST_EQUAL("cms", cms::name());
    }
    void OnVersion() {
        APSARA_TEST_EQUAL(1000000, cms::version());
    }
    void OnCpu() {
        cms::cpu();
    }
};


UNIT_TEST_CASE(CmsMainUnittest, OnName)
UNIT_TEST_CASE(CmsMainUnittest, OnVersion)
UNIT_TEST_CASE(CmsMainUnittest, OnCpu)

} // namespace logtail

UNIT_TEST_MAIN
