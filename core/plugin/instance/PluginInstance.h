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
#include <utility>
#include <vector>

#include "pipeline/PipelineContext.h"
#include "pipeline/PipelineConfig.h"
#include "monitor/LogtailMetric.h"

namespace logtail {

class PluginInstance {
public:
    struct PluginMeta {
        std::string pluginID;
        std::string childPluginID;
    };
    PluginInstance(const PluginMeta& pluginMeta) : mMeta(pluginMeta) {}
    virtual ~PluginInstance() = default;

    const PluginMeta& Meta() const { return mMeta; }

    virtual const std::string& Name() const = 0;
    virtual bool Init(const ComponentConfig& config, PipelineContext& context) = 0;

protected:
    const PluginMeta mMeta;
};

} // namespace logtail
