
#include "PluginMetricManager.h"
#include "monitor/LoongCollectorMetricTypes.h"

namespace logtail {

class PromSelfMonitor {
public:
    bool Init() {
        static const std::unordered_map<std::string, MetricType> inputFileMetricKeys = {};
        mPluginMetricManager = std::make_shared<PluginMetricManager>(mGlobalLabels, inputFileMetricKeys);
        return true;
    }
    void Stop();

private:
    MetricLabels mGlobalLabels;

    PluginMetricManagerPtr mPluginMetricManager;
};

} // namespace logtail