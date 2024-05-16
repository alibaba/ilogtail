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

#include <optional>
#include <re2/re2.h>
#include "models/PipelineEventGroup.h"
#include "models/MetricEvent.h"

using namespace std;
using namespace logtail;

namespace prom
{

const string SAMPLE_RE = R"""(^(?P<name>\w+)(\{(?P<labels>[^}]+)\})?\s+(?P<value>\S+)(\s+(?P<timestamp>\S+))?)""";

class TextParser {
    public:
        TextParser(shared_ptr<SourceBuffer> sourceBuffer): mSourceBuffer(sourceBuffer), mSampleRegex(SAMPLE_RE) {
            if (!mSampleRegex.ok()) {
                throw runtime_error("Invalid regex");
            }
        }
        unique_ptr<PipelineEventGroup> Parse(const string& content);
        unique_ptr<PipelineEventGroup> Parse(const string& content, const time_t defaultTs);
    private:
        shared_ptr<SourceBuffer> mSourceBuffer;
        RE2 mSampleRegex;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class TextParserUnittest;
#endif
};

} // namespace prom
