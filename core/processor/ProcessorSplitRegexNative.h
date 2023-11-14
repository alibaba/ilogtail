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

#include <cstdint>

#include "boost/regex.hpp"

#include "plugin/interface/Processor.h"

namespace logtail {

class ProcessorSplitRegexNative : public Processor {
public:
    static const std::string sName;

    std::string mSplitKey;
    std::string mStartPattern;
    std::string mContinuePattern;
    std::string mEndPattern;
    std::shared_ptr<boost::regex> mStartPatternRegPtr;
    std::shared_ptr<boost::regex> mContinuePatternRegPtr;
    std::shared_ptr<boost::regex> mEndPatternRegPtr;
    bool mIsMultline;
    bool mAppendingLogPositionMeta = false;
    bool mKeepingSourceWhenParseFail = false;

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config) override;
    void Process(PipelineEventGroup& logGroup) override;

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) const override;

private:
    bool IsMultiline() const;
    bool ParseRegex(const std::string& pattern, std::shared_ptr<boost::regex>& reg);
    void ProcessEvent(PipelineEventGroup& logGroup,
                      const StringView& logPath,
                      const PipelineEventPtr& e,
                      EventsContainer& newEvents);
    bool LogSplit(const char* buffer,
                  int32_t size,
                  int32_t& lineFeed,
                  std::vector<StringView>& logIndex,
                  std::vector<StringView>& discardIndex,
                  const StringView& logPath);
    void SetLogMultilinePolicy(const std::string& begReg, const std::string& conReg, const std::string& endReg);
    void HandleUnmatchLogs(const char* buffer,
                           int& multiBeginIndex,
                           int endIndex,
                           std::vector<StringView>& logIndex,
                           std::vector<StringView>& discardIndex);

    int* mFeedLines = nullptr;
    int* mSplitLines = nullptr;

    std::string mLogBeginReg;
    std::string mLogContinueReg;
    std::string mLogEndReg;
    std::unique_ptr<boost::regex> mLogBeginRegPtr;
    std::unique_ptr<boost::regex> mLogContinueRegPtr;
    std::unique_ptr<boost::regex> mLogEndRegPtr;
#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorSplitRegexNativeUnittest;
    friend class ProcessorSplitRegexDisacardUnmatchUnittest;
    friend class ProcessorSplitRegexKeepUnmatchUnittest;
#endif
};

} // namespace logtail