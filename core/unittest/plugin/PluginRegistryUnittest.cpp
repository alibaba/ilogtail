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

#include "plugin/PluginRegistry.h"
#include "plugin/creator/StaticFlusherCreator.h"
#include "plugin/creator/StaticInputCreator.h"
#include "plugin/creator/StaticProcessorCreator.h"
#include "unittest/plugin/PluginMock.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class PluginRegistryUnittest : public testing::Test {
public:
    void TestCreateInput() const;
    void TestCreateProcessor() const;
    void TestCreateFlusher() const;
    void TestValidPlugin() const;

protected:
    void SetUp() override {
        PluginRegistry::GetInstance()->RegisterInputCreator(new StaticInputCreator<InputMock>());
        PluginRegistry::GetInstance()->RegisterProcessorCreator(new StaticProcessorCreator<ProcessorMock>());
        PluginRegistry::GetInstance()->RegisterFlusherCreator(new StaticFlusherCreator<FlusherMock>());
    }
    void TearDown() override { PluginRegistry::GetInstance()->UnloadPlugins(); }
};

void PluginRegistryUnittest::TestCreateInput() const {
    unique_ptr<InputInstance> input = PluginRegistry::GetInstance()->CreateInput(InputMock::sName, "0");
    APSARA_TEST_NOT_EQUAL_FATAL(nullptr, input);
    APSARA_TEST_EQUAL("0", input->Id());
}

void PluginRegistryUnittest::TestCreateProcessor() const {
    unique_ptr<ProcessorInstance> processor = PluginRegistry::GetInstance()->CreateProcessor(ProcessorMock::sName, "0");
    APSARA_TEST_NOT_EQUAL_FATAL(nullptr, processor);
    APSARA_TEST_EQUAL("0", processor->Id());
}

void PluginRegistryUnittest::TestCreateFlusher() const {
    unique_ptr<FlusherInstance> flusher = PluginRegistry::GetInstance()->CreateFlusher(FlusherMock::sName, "0");
    APSARA_TEST_NOT_EQUAL_FATAL(nullptr, flusher);
    APSARA_TEST_EQUAL("0", flusher->Id());
}

void PluginRegistryUnittest::TestValidPlugin() const {
    APSARA_TEST_TRUE(PluginRegistry::GetInstance()->IsValidNativeInputPlugin("input_mock"));
    APSARA_TEST_FALSE(PluginRegistry::GetInstance()->IsValidNativeInputPlugin("input_unknown"));
    APSARA_TEST_TRUE(PluginRegistry::GetInstance()->IsValidNativeProcessorPlugin("processor_mock"));
    APSARA_TEST_FALSE(PluginRegistry::GetInstance()->IsValidNativeProcessorPlugin("processor_unknown"));
    APSARA_TEST_TRUE(PluginRegistry::GetInstance()->IsValidNativeFlusherPlugin("flusher_mock"));
    APSARA_TEST_FALSE(PluginRegistry::GetInstance()->IsValidNativeFlusherPlugin("flusher_unknown"));
    APSARA_TEST_TRUE(PluginRegistry::GetInstance()->IsValidGoPlugin("service_mock"));
    APSARA_TEST_TRUE(PluginRegistry::GetInstance()->IsValidGoPlugin("service_unknown"));
}

UNIT_TEST_CASE(PluginRegistryUnittest, TestCreateInput)
UNIT_TEST_CASE(PluginRegistryUnittest, TestCreateProcessor)
UNIT_TEST_CASE(PluginRegistryUnittest, TestCreateFlusher)
UNIT_TEST_CASE(PluginRegistryUnittest, TestValidPlugin)

} // namespace logtail

UNIT_TEST_MAIN
