#include "plugin/instance/InputInstance.h"

namespace logtail {
bool InputInstance::Init(const Table& config, PipelineContext& context) {
    mContext = &context;
    mPlugin->SetContext(context);
    if (mPlugin->Init(config)) {
        return false;
    }
    return true;
}
} // namespace logtail