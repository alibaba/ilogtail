// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
    } else if (!mode.empty() && mode != "custom") {
        PARAM_WARNING_DEFAULT(ctx.GetLogger(), errorMsg, "custom", pluginName, ctx.GetConfigName());
    }

    if (mMode == Mode::CUSTOM) {
        // StartPattern
        string pattern;
        if (!GetOptionalStringParam(config, "Multiline.StartPattern", pattern, errorMsg)) {
            PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
        } else if (!ParseRegex(pattern, mStartPatternRegPtr)) {
            PARAM_WARNING_IGNORE(
                ctx.GetLogger(), "Multiline.StartPattern is not a valid regex", pluginName, ctx.GetConfigName());
        } else {
            mStartPattern = pattern;
        }

        // ContinuePattern
        pattern.clear();
        if (!GetOptionalStringParam(config, "Multiline.ContinuePattern", pattern, errorMsg)) {
            PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
        } else if (!ParseRegex(pattern, mContinuePatternRegPtr)) {
            PARAM_WARNING_IGNORE(
                ctx.GetLogger(), "Multiline.ContinuePattern is not a valid regex", pluginName, ctx.GetConfigName());
        } else {
            mContinuePattern = pattern;
        }

        // EndPattern
        pattern.clear();
        if (!GetOptionalStringParam(config, "Multiline.EndPattern", pattern, errorMsg)) {
            PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
        } else if (!ParseRegex(pattern, mEndPatternRegPtr)) {
            PARAM_WARNING_IGNORE(
                ctx.GetLogger(), "Multiline.EndPattern is not a valid regex", pluginName, ctx.GetConfigName());
        } else {
            mEndPattern = pattern;
        }

        if (!mStartPatternRegPtr && !mEndPatternRegPtr && mContinuePatternRegPtr) {
            mContinuePatternRegPtr.reset();
            LOG_WARNING(ctx.GetLogger(),
                        ("problem encountered in config parsing",
                         "param Multiline.StartPattern and EndPattern are empty but ContinuePattern is not")(
                            "action", "ignore multiline config")("module", pluginName)("config", ctx.GetConfigName()));
        } else if (mStartPatternRegPtr || mEndPatternRegPtr) {
            mIsMultiline = true;
        }
    }

    return true;
}

bool MultilineOptions::ParseRegex(const string& pattern, shared_ptr<boost::regex>& reg) {
    if (pattern.empty() || pattern == ".*") {
        return true;
    }
    try {
        reg.reset(new boost::regex(pattern));
    } catch (...) {
        return false;
    }
    return true;
}

} // namespace logtail
