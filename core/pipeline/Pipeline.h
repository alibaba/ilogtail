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

#include <string>
#include <vector>
#include <memory>

// #include "config/NewConfig.h"
#include "models/PipelineEventGroup.h"
#include "pipeline/PipelineContext.h"
#include "plugin/instance/InputInstance.h"
#include "plugin/instance/FlusherInstance.h"
#include "plugin/instance/ProcessorInstance.h"
// #include "table/JSONTable.h"
#include "json/json.h"

namespace logtail {

class Pipeline {
public:
    bool Init(const PipelineConfig& config);
    // bool Init(NewConfig&& config);
    bool Init(const Json::Value& config);
    void Start();
    void Process(PipelineEventGroup& logGroup);
    void Stop(bool isRemoving);
    const std::string& Name() const { return mName; }
    // const NewConfig& GetConfig() const { return mConfig; }
    PipelineContext& GetContext() { return mContext; }
    bool LoadGoPipelines() const; // 应当放在private，过渡期间放在public
    PipelineConfig& GetPipelineConfig() { return mConfig; }
    Json::Value& GetGoPipelineWithInput() { return mGoPipelineWithInput; }
    Json::Value& GetGoPipelineWithoutInput() { return mGoPipelineWithoutInput; }

private:
    bool InitAndAddProcessor(std::unique_ptr<ProcessorInstance>&& processor, const PipelineConfig& config);

    std::string mName;
    std::vector<std::unique_ptr<InputInstance>> mInputs;
    std::vector<std::unique_ptr<ProcessorInstance>> mProcessorLine;
    std::vector<std::unique_ptr<FlusherInstance>> mFlushers;
    Json::Value mGoPipelineWithInput;
    Json::Value mGoPipelineWithoutInput;
    PipelineContext mContext;
    PipelineConfig mConfig;

    bool mRequiringSpecialStopOrder = false;
};
} // namespace logtail
