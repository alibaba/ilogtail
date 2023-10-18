#include "plugin/instance/FlusherInstance.h"

namespace logtail {
bool FlusherInstance::Init(const Json::Value& config, PipelineContext& context, Json::Value &optionalGoPipeline) {
    mPlugin->SetContext(context);
    mPlugin->SetMetricsRecordRef(Name(), Id());
    if (mPlugin->Init(config, optionalGoPipeline)) {
        return false;
    }
    return true;
}
} // namespace logtail
