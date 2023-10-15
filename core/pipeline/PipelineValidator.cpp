#include "pipeline/PipelineValidator.h"

using namespace std;

namespace logtail {
void PipelineValidator::Init() {
    mFuncs.push_back(ValidateShape);
}

bool PipelineValidator::Validate(const std::shared_ptr<Pipeline>& p) {
    for (auto& f : mFuncs) {
        if (!f(p)) {
            return false;
        }
    }
    return true;
}

// TODO: undefined
bool PipelineValidator::ValidateShape(const std::shared_ptr<Pipeline>& p) {
    return true;
}
} // namespace logtail
