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

#include <cstdlib>
#include "unittest/Unittest.h"
#include "common/Constants.h"
#include "config/Config.h"
#include "config_manager/ConfigManager.h"
#include "processor/ProcessorFillSlsGroupInfo.h"

namespace logtail {

class ProcessorFillSlsGroupInfoUnittest : public ::testing::Test {
public:
    static void SetUpTestCase() { ConfigManager::GetInstance()->SetUserDefinedIdSet({"mg1", "mg2"}); }

    void SetUp() override {
        mContext.SetConfigName("project##config_0");
        mContext.SetLogstoreName("logstore");
        mContext.SetProjectName("project");
        mContext.SetRegion("cn-shanghai");
    }

    void TestInit();
    void TestGetTopicName();
    void TestProcess();

    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorFillSlsGroupInfoUnittest, TestInit);
UNIT_TEST_CASE(ProcessorFillSlsGroupInfoUnittest, TestGetTopicName);
UNIT_TEST_CASE(ProcessorFillSlsGroupInfoUnittest, TestProcess);

void ProcessorFillSlsGroupInfoUnittest::TestInit() {
    Config config;
    config.mLogType = REGEX_LOG;
    config.mTopicFormat = "default";
    config.mGroupTopic = "group_topic";
    config.mCustomizedTopic = "static_topic";

    {
        ProcessorFillSlsGroupInfo processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        APSARA_TEST_EQUAL_FATAL("none", processor.mTopicFormat);
        APSARA_TEST_EQUAL_FATAL(config.mGroupTopic, processor.mGroupTopic);
        APSARA_TEST_EQUAL_FATAL(std::string(), processor.mStaticTopic);
    }

    {
        config.mLogType = APSARA_LOG;
        ProcessorFillSlsGroupInfo processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        APSARA_TEST_EQUAL_FATAL("default", processor.mTopicFormat);
    }
}

void ProcessorFillSlsGroupInfoUnittest::TestGetTopicName() {
    Config config;
    config.mLogType = REGEX_LOG;
    config.mGroupTopic = "group_topic";
    config.mCustomizedTopic = "static_topic";
    std::vector<sls_logs::LogTag> tags;
    {
        config.mTopicFormat = "/var/log/(.*)";
        ProcessorFillSlsGroupInfo processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        tags.clear();
        APSARA_TEST_EQUAL_FATAL(std::string("message"), processor.GetTopicName("/var/log/message", tags));
        APSARA_TEST_EQUAL_FATAL(0L, tags.size());
    }

    {
        config.mTopicFormat = "/var/(.*)/(.*)";
        ProcessorFillSlsGroupInfo processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        tags.clear();
        APSARA_TEST_EQUAL_FATAL(std::string("log_message"), processor.GetTopicName("/var/log/message", tags));
        APSARA_TEST_EQUAL_FATAL(2L, tags.size());
        APSARA_TEST_EQUAL_FATAL(std::string("__topic_1__"), tags[0].key());
        APSARA_TEST_EQUAL_FATAL(std::string("log"), tags[0].value());
        APSARA_TEST_EQUAL_FATAL(std::string("__topic_2__"), tags[1].key());
        APSARA_TEST_EQUAL_FATAL(std::string("message"), tags[1].value());
    }
}

void ProcessorFillSlsGroupInfoUnittest::TestProcess() {
    Config config;
    config.mLogType = REGEX_LOG;
    config.mTopicFormat = "none";
    config.mGroupTopic = "group_topic";
    config.mCustomizedTopic = "static_topic";

    ProcessorFillSlsGroupInfo processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));

    auto sourceBuffer = std::make_shared<logtail::SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    processor.Process(eventGroup);
    APSARA_TEST_TRUE_FATAL(eventGroup.HasMetadata(EVENT_META_AGENT_TAG));
    APSARA_TEST_TRUE_FATAL(eventGroup.HasMetadata(EVENT_META_HOST_IP));
    APSARA_TEST_TRUE_FATAL(eventGroup.HasMetadata(EVENT_META_HOST_NAME));
    APSARA_TEST_FALSE_FATAL(eventGroup.HasMetadata(EVENT_META_LOG_FILE_PATH));
    APSARA_TEST_FALSE_FATAL(eventGroup.HasMetadata(EVENT_META_LOG_FILE_PATH_RESOLVED));
}

} // namespace logtail

UNIT_TEST_MAIN