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

#include "unittest/Unittest.h"
#include "event_handler/HistoryFileImporter.h"

namespace logtail {
class HistoryFileImporterUnittest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}

public:
    void TestFlowControl() {
        auto importer = HistoryFileImporter::GetInstance();
        auto now = GetCurrentTimeInMicroSeconds();
        importer->lastPushBufferTime = now;
        importer->FlowControl(10, 1000000);
        APSARA_TEST_GE_FATAL(GetCurrentTimeInMicroSeconds() - now, 10);
    }
};

APSARA_UNIT_TEST_CASE(HistoryFileImporterUnittest, TestFlowControl, 0);
} // end of namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
