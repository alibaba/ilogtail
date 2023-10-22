#include "file_server/MultilineOptions.h"

#include "common/ParamExtractor.h"

using namespace std;

namespace logtail {
bool MultilineOptions::Init(const Json::Value& config, const PipelineContext& ctx, const string& pluginName) {
    string errorMsg;
    // Mode
    string mode;
    if (!GetOptionalStringParam(config, "Multiline.Mode", mode, errorMsg)) {
        PARAM_WARNING_DEFAULT(ctx.GetLogger(), errorMsg, "custom", pluginName, ctx.GetConfigName());
    } else if (mode == "JSON") {
        mMode = Mode::JSON;
        mIsMultiline = true;
    } else if (mode != "custom") {
        PARAM_WARNING_DEFAULT(ctx.GetLogger(), errorMsg, "custom", pluginName, ctx.GetConfigName());
    }

    if (mMode == Mode::CUSTOM) {
        // StartPattern
        string pattern;
        if (!GetOptionalStringParam(config, "Multiline.StartPattern", pattern, errorMsg)) {
            PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
        } else if (!IsRegexValid(pattern)) {
            PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
        } else {
            mStartPattern = pattern;
        }

        // ContinuePattern
        if (!GetOptionalStringParam(config, "Multiline.ContinuePattern", pattern, errorMsg)) {
            PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
        } else if (!IsRegexValid(pattern)) {
            PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
        } else {
            mContinuePattern = pattern;
        }

        // EndPattern
        if (!GetOptionalStringParam(config, "Multiline.EndPattern", pattern, errorMsg)) {
            PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
        } else if (!IsRegexValid(pattern)) {
            PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
        } else {
            mEndPattern = pattern;
        }

        if ((mStartPattern.empty() || mStartPattern == ".*")
            && (mEndPattern.empty() || mEndPattern == ".*")
            && !mContinuePattern.empty()) {
            LOG_WARNING(ctx.GetLogger(),
                        ("problem encountered in config parsing",
                         "param Multiline.StartPattern and EndPattern are empty but ContinuePattern is not")(
                            "action", "ignore multiline config")("module", pluginName)("config", ctx.GetConfigName()));
        } else if ((!mStartPattern.empty() && mStartPattern != ".*")
                   || (!mEndPattern.empty() && mEndPattern != ".*")) {
            mIsMultiline = true;
        }
    }

    return true;
}
} // namespace logtail
