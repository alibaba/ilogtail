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

#include "processor/ProcessorInterface.h"
#include <string>
#include <boost/regex.hpp>

namespace logtail {

class ProcessorSplitRegexNative : public ProcessorInterface {
public:
    static const char* Name() { return "processor_split_regex_native"; }
    bool Init(const ComponentConfig& config, PipelineContext& context) override;
    void Process(PipelineEventGroup& logGroup) override;

private:
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

    PipelineContext mContext;
    int* mFeedLines = nullptr;
    int* mSplitLines = nullptr;
    std::string mSplitKey;
    std::string mLogBeginReg;
    std::unique_ptr<boost::regex> mLogBeginRegPtr;
    bool mDiscardUnmatch;
    bool mEnableLogPositionMeta;
};

} // namespace logtail