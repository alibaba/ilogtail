#pragma once

#include <string>

#include "json/json.h"

#include "pipeline/PipelineContext.h"

namespace logtail {

class MultilineOptions {
public:
    enum class Mode { CUSTOM, JSON };

    bool Init(const Json::Value& config, const PipelineContext& ctx, const std::string& pluginName);
    bool IsMultiline() const { return mIsMultiline; }

    Mode mMode = Mode::CUSTOM;
    std::string mStartPattern;
    std::string mContinuePattern;
    std::string mEndPattern;

private:
    bool mIsMultiline = false;
};

} // namespace logtail
