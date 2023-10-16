#pragma once

#include <string>
#include <utility>
#include <vector>

#include "pipeline/PipelineContext.h"
#include "monitor/LogtailMetric.h"

namespace logtail {
class Plugin {
public:
    virtual ~Plugin() = default;

    virtual const std::string& Name() const = 0;
    void SetContext(PipelineContext& context) { mContext = &context; }
    PipelineContext& GetContext() { return *mContext; }
    MetricsRecordRef& GetMetricsRecordRef() { return mMetricsRecordRef; }

protected:
    void SetMetricsRecordRef(const std::string& name, const std::string& id) {
        std::vector<std::pair<std::string, std::string>> labels;
        WriteMetrics::GetInstance()->PreparePluginCommonLabels(GetContext().GetProjectName(),
                                                               GetContext().GetLogstoreName(),
                                                               GetContext().GetRegion(),
                                                               GetContext().GetConfigName(),
                                                               name,
                                                               id,
                                                               labels);

        WriteMetrics::GetInstance()->PrepareMetricsRecordRef(mMetricsRecordRef, std::move(labels));
    }

    PipelineContext* mContext = nullptr;
    MetricsRecordRef mMetricsRecordRef;
};
} // namespace logtail
