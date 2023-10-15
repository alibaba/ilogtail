#include "plugin/instance/FlusherInstance.h"

namespace logtail {
bool FlusherInstance::Init(const Table& config, PipelineContext& context) {
    mContext = &context;
    mPlugin->SetContext(context);
    if (mPlugin->Init(config)) {
        return false;
    }
    return true;
}
} // namespace logtail