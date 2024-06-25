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
#include "unittest/Unittest.h"  // 这个文件跟<fmt/printf.h>冲突
#include "cms/common/ExceptionsHandler.h"
#include "cms/CmsMain.h"
#include "cms/common/UnitTestEnv.h"

const boost::filesystem::path TEST_CONF_PATH     = boost::filesystem::path(__FILE__).parent_path() / "unittest_data" / "other";
const boost::filesystem::path TEST_SIC_CONF_PATH = boost::filesystem::path(__FILE__).parent_path() / "unittest_data" / "sic";

namespace logtail {

// class CmsForOneAgentUnittest : public testing::Test {
// public:
//     void OnName() {
//         APSARA_TEST_EQUAL("cms", cms::name());
//     }
//     void OnVersion() {
//         APSARA_TEST_EQUAL(1000000, cms::version());
//     }
//     void OnCpu() {
//         cms::cpu();
//     }
// };


// UNIT_TEST_CASE(CmsForOneAgentUnittest, OnName)
// UNIT_TEST_CASE(CmsForOneAgentUnittest, OnVersion)
// UNIT_TEST_CASE(CmsForOneAgentUnittest, OnCpu)

} // namespace logtail

int main(int argc, char** argv) {
    SetupProcess();
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
