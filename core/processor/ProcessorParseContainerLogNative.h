/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "common/TimeUtil.h"
#include "models/LogEvent.h"
#include "plugin/interface/Processor.h"
#include "processor/CommonParserOptions.h"

namespace logtail {

struct ContainerStdoutKeyStringBuffer {
    StringBuffer timeKeyBuffer;
    StringBuffer sourceKeyBuffer;
    StringBuffer logKeyBuffer;
    StringBuffer flagBuffer;
};

class ProcessorParseContainerLogNative : public Processor {
public:
    static const std::string sName;

    static const char CONTIANERD_DELIMITER; // 分隔符
    static const char CONTIANERD_FULL_TAG; // 容器全标签
    static const char CONTIANERD_PART_TAG; // 容器部分标签

    static const std::string DOCKER_JSON_LOG; // docker json 日志字段
    static const std::string DOCKER_JSON_TIME; // docker json 时间字段
    static const std::string DOCKER_JSON_STREAM_TYPE; // docker json 流字段

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config) override;
    void Process(PipelineEventGroup& logGroup) override;

    // Source field name.
    std::string mSourceKey = DEFAULT_CONTENT_KEY;
    bool mIgnoringStdout = false;
    bool mIgnoringStderr = false;

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) const override;

private:
    std::string containerTimeKey = "_time_"; // 容器时间字段
    std::string containerSourceKey = "_source_"; // 容器来源字段
    std::string containerLogKey = "content"; // 容器日志字段

    bool ProcessEvent(PipelineEventGroup& logGroup,
                      const StringView containerType,
                      PipelineEventPtr& e,
                      ContainerStdoutKeyStringBuffer* containerStdoutKeyStringBuffer);
    void AddDockerJsonLog(char** data, const StringBuffer key, const StringView value, LogEvent& targetEvent);
    void AddContainerdTextLog(ContainerStdoutKeyStringBuffer* keyBuffer,
                 StringView timeValue,
                 StringView sourceValue,
                 StringView content,
                 StringView partTag,
                 LogEvent& sourceEvent);
    bool ContainerdLogLineParser(LogEvent& sourceEvent, ContainerStdoutKeyStringBuffer* containerStdoutKeyStringBuffer);
    bool DockerJsonLogLineParser(PipelineEventGroup& logGroup,
                                 LogEvent& sourceEvent,
                                 ContainerStdoutKeyStringBuffer* containerStdoutKeyStringBuffer);
#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorParseContainerLogNativeUnittest;
#endif
};

} // namespace logtail
