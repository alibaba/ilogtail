/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <memory>

#include "plugin/instance/PluginInstance.h"
#include "plugin/interface/Flusher.h"
// #include "table/Table.h"

namespace logtail {

class FlusherInstance: public PluginInstance {
public:
    FlusherInstance(Flusher* plugin, const std::string& pluginId) : PluginInstance(pluginId), mPlugin(plugin) {}

    const std::string& Name() const override { return mPlugin->Name(); };
    // bool Init(const Table& config, PipelineContext& context);
    bool Init(const Json::Value& config, PipelineContext& context);
    void Start() { mPlugin->Start(); }
    void Stop(bool isPipelineRemoving) { mPlugin->Stop(isPipelineRemoving); }

private:
    std::unique_ptr<Flusher> mPlugin;
};

}
