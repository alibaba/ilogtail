/*
 * Copyright 2024 iLogtail Authors
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

#include <re2/re2.h>
#include "models/PipelineEventGroup.h"
#include "models/MetricEvent.h"

namespace logtail
{

extern const std::string SAMPLE_RE;

class TextParser {
    public:
        TextParser(const std::shared_ptr<SourceBuffer>& sourceBuffer): mSourceBuffer(sourceBuffer), mSampleRegex(SAMPLE_RE) {
            if (!mSampleRegex.ok()) {
                mErr = std::make_shared<std::exception>(std::invalid_argument("invalid regex"));
            }
        }
        std::unique_ptr<PipelineEventGroup> Parse(const std::string& content);
        std::unique_ptr<PipelineEventGroup> Parse(const std::string& content, std::time_t defaultTs);

        bool Ok() const;
        std::shared_ptr<std::exception> Err() const;
    private:
        std::shared_ptr<std::exception> mErr;
        std::shared_ptr<SourceBuffer> mSourceBuffer;
        RE2 mSampleRegex;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class TextParserUnittest;
#endif
};

} // namespace logtail
