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

#include <vector>
#include <unordered_map>

#include "monitor/LogtailMetric.h"
#include "plugin/interface/Processor.h"

namespace logtail {

class ProcessorDesensitizeNative : public Processor {
public:
    static const std::string sName;

    const std::string& Name() const override { return sName; }
    bool Init(const ComponentConfig& componentConfig) override;
    void Process(PipelineEventGroup& logGroup) override;

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) const override;

private:
    void ProcessEvent(PipelineEventPtr& e);
    void CastOneSensitiveWord(const std::vector<SensitiveWordCastOption>& optionVec, std::string* value);

    std::unordered_map<std::string, std::vector<SensitiveWordCastOption>> mSensitiveWordCastOptions;

    CounterPtr mProcDesensitizeRecodesTotal;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorParseApsaraNativeUnittest;
#endif
};

} // namespace logtail