#include "plugin/instance/FlusherInstance.h"

namespace logtail {
bool FlusherInstance::Init(const Table& config, PipelineContext& context) {
    mPlugin->SetContext(context);
    mPlugin->SetMetricsRecordRef(Name(), Id());
    if (mPlugin->Init(config)) {
        return false;
    }
    return true;
}
} // namespace logtail
