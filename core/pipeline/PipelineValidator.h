#pragma once

#include <memory>
#include <vector>
#include <functional>

#include "pipeline/Pipeline.h"

namespace logtail {

class PipelineValidator {
public:
    PipelineValidator(const PipelineValidator&) = delete;
    PipelineValidator& operator=(const PipelineValidator&) = delete;

    static PipelineValidator* GetInstance() {
        static PipelineValidator instance;
        return &instance;
    }

    void Init();
    bool Validate(const std::shared_ptr<Pipeline>& p);

private:
    PipelineValidator() = default;
    ~PipelineValidator() = default;

    static bool ValidateShape(const std::shared_ptr<Pipeline>& p); // TODO: undefined

    std::vector<std::function<bool(const std::shared_ptr<Pipeline>&)>> mFuncs;
};
} // namespace logtail
