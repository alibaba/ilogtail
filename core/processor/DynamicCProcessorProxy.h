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

#pragma once

#include <string>
#include "plugin/interface/Processor.h"
#include "plugin/creator/CProcessor.h"

namespace logtail {

class DynamicCProcessorProxy : public Processor {
public:
    DynamicCProcessorProxy(const char* name);
    ~DynamicCProcessorProxy();

    const std::string& Name() const override { return _name; }
    bool Init(const ComponentConfig& componentConfig) override;
    void Process(PipelineEventGroup& logGroup) override;
    void SetCProcessor(const processor_interface_t* c_ins);

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) const override;

private:
    std::string _name;
    processor_instance_t* _c_ins;
};
} // namespace logtail