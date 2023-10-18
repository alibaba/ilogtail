#include "plugin/instance/InputInstance.h"

namespace logtail {
bool InputInstance::Init(const Json::Value& config, PipelineContext& context, Json::Value &optionalGoPipeline) {
    mPlugin->SetContext(context);
    mPlugin->SetMetricsRecordRef(Name(), Id());
    if (mPlugin->Init(config, optionalGoPipeline)) {
        return false;
    }
    return true;
}
} // namespace logtail
