/*
 * Copyright 2022 iLogtail Authors
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

#include "plugin/ProcessorInstance.h"

namespace logtail {

bool ProcessorInstance::Init(const ComponentConfig& config, PipelineContext& context) {
    mContext = context;
    return mPlugin->Init(config, context);
}

void ProcessorInstance::Process(PipelineEventGroup& logGroup) {
    size_t inSize = logGroup.GetEvents().size();
    mPlugin->Process(logGroup);
    size_t outSize = logGroup.GetEvents().size();
    LOG_DEBUG(mContext.GetLogger(), ("Processor", Id())("InSize", inSize)("OutSize", outSize));
}

} // namespace logtail