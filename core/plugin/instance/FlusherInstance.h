#pragma once

#include <memory>

#include "flusher/Flusher.h"
#include "plugin/instance/PluginInstance.h"
#include "table/Table.h"

namespace logtail {
class FlusherInstance: public PluginInstance {
public:
    FlusherInstance(Flusher* plugin, const std::string& pluginId) : PluginInstance(pluginId), mPlugin(plugin) {}

    bool Init(const Table& config, PipelineContext& context);
    void Start() { mPlugin->Start(); }
    void Stop(bool isPipelineRemoving) { mPlugin->Stop(isPipelineRemoving); }

private:
    std::unique_ptr<Flusher> mPlugin;
};
}