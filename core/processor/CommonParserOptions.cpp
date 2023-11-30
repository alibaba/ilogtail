/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "processor/CommonParserOptions.h"

#include "common/ParamExtractor.h"

namespace logtail {

bool CommonParserOptions::Init(const Json::Value& config, const PipelineContext& ctx, const std::string& pluginName) {
    std::string errorMsg;
    if (!GetOptionalBoolParam(config, "KeepingSourceWhenParseFail", mKeepingSourceWhenParseFail, errorMsg)) {
        PARAM_WARNING_DEFAULT(ctx.GetLogger(), errorMsg, mKeepingSourceWhenParseFail, pluginName, ctx.GetConfigName());
    }
    if (!GetOptionalBoolParam(config, "KeepingSourceWhenParseSucceed", mKeepingSourceWhenParseSucceed, errorMsg)) {
        PARAM_WARNING_DEFAULT(
            ctx.GetLogger(), errorMsg, mKeepingSourceWhenParseSucceed, pluginName, ctx.GetConfigName());
    }
    if (!GetOptionalStringParam(config, "RenamedSourceKey", mRenamedSourceKey, errorMsg)) {
        PARAM_WARNING_DEFAULT(ctx.GetLogger(), errorMsg, mRenamedSourceKey, pluginName, ctx.GetConfigName());
    }
    if (!GetOptionalBoolParam(config, "CopingRawLog", mCopingRawLog, errorMsg)) {
        PARAM_WARNING_DEFAULT(ctx.GetLogger(), errorMsg, mCopingRawLog, pluginName, ctx.GetConfigName());
    }
    return true;
}
bool CommonParserOptions::ShouldAddUnmatchLog(bool parseSuccess) {
    return !parseSuccess && mKeepingSourceWhenParseFail && mCopingRawLog;
}

// Parsing successful and original logs are retained or parsing failed and original logs are retained.
bool CommonParserOptions::ShouldAddRenamedSourceLog(bool parseSuccess) {
    return (((parseSuccess && mKeepingSourceWhenParseSucceed) || (!parseSuccess && mKeepingSourceWhenParseFail)));
}
bool CommonParserOptions::ShouldEraseEvent(bool parseSuccess, const LogEvent& sourceEvent) {
    if (!parseSuccess && !mKeepingSourceWhenParseFail) {
        const auto& contents = sourceEvent.GetContents();
        if (contents.empty()) {
            return true;
        }
        // "__file_offset__"
        if (contents.size() == 1 && (contents.begin()->first == LOG_RESERVED_KEY_FILE_OFFSET)) {
            return true;
        }
    }
    return false;
}
} // namespace logtail
