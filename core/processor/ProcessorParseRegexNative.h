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
#include "processor/ProcessorInterface.h"
#include <vector>
#include "parser/LogParser.h" //UserDefinedFormat

namespace logtail {

class ProcessorParseRegexNative : public ProcessorInterface {
public:
    static const char* Name() { return "processor_parse_regex_native"; }
    bool Init(const ComponentConfig& config, PipelineContext& context) override;
    void Process(PipelineEventGroup& logGroup) override;

private:
    void AddUserDefinedFormat(const std::string& regStr, const std::string& keys);
    /// @return false if data need to be discarded
    bool ProcessEvent(PipelineEventGroup& logGroup, const StringView& logPath, PipelineEventPtr& e);
    bool WholeLineModeParser(const LogEvent& sourceEvent, const std::string& key, LogEvent& targetEvent);
    bool RegexLogLineParser(const LogEvent& sourceEvent,
                            const boost::regex& reg,
                            const std::vector<std::string>& keys,
                            const StringView& logPath,
                            LogEvent& targetEvent);
    void AddLog(const StringView& key, const StringView& value, LogEvent& targetEvent);
    std::string mSourceKey;
    std::vector<UserDefinedFormat> mUserDefinedFormat;
    bool mDiscardUnmatch;
    bool mUploadRawLog;
    bool mSourceKeyOverwritten;
    bool mRawLogTagOverwritten;
    std::string mRawLogTag;

    PipelineContext mContext;
    int* mParseFailures;
    int* mRegexMatchFailures;
    int* mLogGroupSize;
};
} // namespace logtail