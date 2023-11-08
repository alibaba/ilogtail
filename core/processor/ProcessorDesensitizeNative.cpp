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
#include "processor/ProcessorDesensitizeNative.h"
#include "common/Constants.h"
#include "models/LogEvent.h"
#include "sdk/Common.h"
#include "plugin/instance/ProcessorInstance.h"
#include "monitor/MetricConstants.h"
#include "common/ParamExtractor.h"


namespace logtail {
const std::string ProcessorDesensitizeNative::sName = "processor_desensitize_native";

bool ProcessorDesensitizeNative::Init(const Json::Value& config) {
    std::string errorMsg;

    if (!GetMandatoryStringParam(config, "SourceKey", mSourceKey, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
    }

    std::string method;
    if (!GetMandatoryStringParam(config, "Method", method, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
    }
    if (method == "const") {
        mMethod = CONST_OPTION;
    } else if (method == "md5") {
        mMethod = MD5_OPTION;
    } else {
        errorMsg = "The method(" + method + ") is invalid";
        PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
    }

    // 当Method取值为const时必选
    if (mMethod == CONST_OPTION) {
        if (!GetMandatoryStringParam(config, "ReplacingString", mReplacingString, errorMsg)) {
            PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
        }
    }
    mReplacingString = std::string("\\1") + mReplacingString;

    if (!GetMandatoryStringParam(
            config, "ContentPatternBeforeReplacedString", mContentPatternBeforeReplacedString, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
    }

    if (!GetMandatoryStringParam(config, "ReplacedContentPattern", mReplacedContentPattern, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(), errorMsg, sName, mContext->GetConfigName());
    }

    std::string regexStr = std::string("(") + mContentPatternBeforeReplacedString + ")" + mReplacedContentPattern;
    mRegex.reset(new re2::RE2(regexStr));
    if (!mRegex->ok()) {
        errorMsg = mRegex->error();
        errorMsg += std::string(", regex : ") + regexStr;
        // do not throw when parse sensitive key error
        mContext->GetAlarm().SendAlarm(CATEGORY_CONFIG_ALARM,
                                       std::string("The sensitive key regex is invalid, ") + errorMsg,
                                       GetContext().GetProjectName(),
                                       GetContext().GetLogstoreName(),
                                       GetContext().GetRegion());
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           "The sensitive regex is invalid, error:" + errorMsg,
                           sName,
                           mContext->GetConfigName());
    }

    if (!GetOptionalBoolParam(config, "ReplacingAll", mReplacingAll, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(), errorMsg, mReplacingAll, sName, mContext->GetConfigName());
    }

    mProcDesensitizeRecodesTotal = GetMetricsRecordRef().CreateCounter(METRIC_PROC_DESENSITIZE_RECORDS_TOTAL);
}

void ProcessorDesensitizeNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty()) {
        return;
    }

    EventsContainer& events = logGroup.MutableEvents();

    for (auto it = events.begin(); it != events.end();) {
        ProcessEvent(*it);
        ++it;
    }
}

void ProcessorDesensitizeNative::ProcessEvent(PipelineEventPtr& e) {
    if (!IsSupportedEvent(e)) {
        return;
    }

    auto& sourceEvent = e.Cast<LogEvent>();

    const LogContents& contents = sourceEvent.GetContents();

    for (auto it = contents.begin(); it != contents.end(); ++it) {
        if (it->first != mSourceKey) {
            continue;
        }
        if (it->second.empty()) {
            continue;
        }
        std::string value = it->second.to_string();
        CastOneSensitiveWord(&value);
        mProcDesensitizeRecodesTotal->Add(1);
        StringBuffer valueBuffer = sourceEvent.GetSourceBuffer()->CopyString(value);
        sourceEvent.SetContentNoCopy(it->first, StringView(valueBuffer.data, valueBuffer.size));
    }
}

void ProcessorDesensitizeNative::CastOneSensitiveWord(std::string* value) {
    std::string* pVal = value;
    bool rst = false;

    if (mMethod == CONST_OPTION) {
        if (mReplacingAll) {
            rst = RE2::GlobalReplace(pVal, *mRegex, mReplacingString);
        } else {
            rst = RE2::Replace(pVal, *mRegex, mReplacingString);
        }
    } else {
        re2::StringPiece srcStr(*pVal);
        size_t maxSize = pVal->size();
        size_t beginPos = 0;
        rst = true;
        std::string destStr;
        do {
            re2::StringPiece findRst;
            if (!re2::RE2::FindAndConsume(&srcStr, *mRegex, &findRst)) {
                if (beginPos == (size_t)0) {
                    rst = false;
                }
                break;
            }
            // like  xxxx, psw=123abc,xx
            size_t beginOffset = findRst.data() + findRst.size() - pVal->data();
            size_t endOffset = srcStr.empty() ? maxSize : srcStr.data() - pVal->data();
            if (beginOffset < beginPos || endOffset <= beginPos || endOffset > maxSize) {
                rst = false;
                break;
            }
            // add : xxxx, psw
            destStr.append(pVal->substr(beginPos, beginOffset - beginPos));
            // md5: 123abc
            destStr.append(sdk::CalcMD5(pVal->substr(beginOffset, endOffset - beginOffset)));
            beginPos = endOffset;
            // refine for  : xxxx. psw=123abc
            if (endOffset >= maxSize) {
                break;
            }

        } while (mReplacingAll);

        if (rst && beginPos < pVal->size()) {
            // add ,xx
            destStr.append(pVal->substr(beginPos));
        }
        if (rst) {
            *value = destStr;
            pVal = value;
        }
    }
}

bool ProcessorDesensitizeNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<LogEvent>();
}

} // namespace logtail