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

#pragma once

#include "plugin/interface/Processor.h"
#include "config/Config.h"

namespace logtail {

class ProcessorDesensitizeNative : public Processor {
public:
    static const std::string sName;

    // 源字段名。
    std::string mSourceKey;
    // 脱敏方式。可选值包括：
    // ● const：用常量替换敏感内容。
    // ● md5：用敏感内容的MD5值替换相应内容。
    int32_t mMethod;
    // 用于替换敏感内容的常量字符串。
    std::string mReplacingString;
    // 敏感内容的前缀正则表达式。
    std::string mContentPatternBeforeReplacedString;
    // 敏感内容的正则表达式
    std::string mReplacedContentPattern;
    // 是否替换所有的匹配的敏感内容。
    bool mReplacingAll = true;

    static const int32_t MD5_OPTION = 0;
    static const int32_t CONST_OPTION = 1;

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config) override;
    void Process(PipelineEventGroup& logGroup) override;

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) const override;

private:
    std::shared_ptr<re2::RE2> mRegex;

    void ProcessEvent(PipelineEventPtr& e);
    void CastOneSensitiveWord(std::string* value);

    CounterPtr mProcDesensitizeRecodesTotal;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorParseApsaraNativeUnittest;
#endif
};

} // namespace logtail