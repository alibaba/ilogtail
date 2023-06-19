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

#include "models/LogEvent.h"
#include <memory>

namespace logtail {

std::string LogEvent::sType = "Log";

const std::string& LogEvent::GetType() const {
    return sType;
};

std::unique_ptr<LogEvent> LogEvent::CreateEvent(std::shared_ptr<SourceBuffer>& sb) {
    auto p = std::unique_ptr<LogEvent>(new LogEvent);
    p->SetSourceBuffer(sb);
    return p;
}

void LogEvent::SetContent(const StringView& key, const StringView& val) {
    SetContent(mSourceBuffer->CopyString(key), mSourceBuffer->CopyString(val));
}
void LogEvent::SetContent(const std::string& key, const std::string& val) {
    SetContent(mSourceBuffer->CopyString(key), mSourceBuffer->CopyString(val));
}
void LogEvent::SetContent(const StringBuffer& key, const StringBuffer& val) {
    SetContentNoCopy(StringView(key.data, key.size), StringView(val.data, val.size));
}

const StringView& LogEvent::GetContent(const std::string& key) const {
    return GetContent(StringView(key));
}

const StringView& LogEvent::GetContent(const StringView& key) const {
    auto it = contents.find(key);
    if (it != contents.end()) {
        return it->second;
    }
    return gEmptyStringView;
}

bool LogEvent::HasContent(const std::string& key) const {
    return HasContent(StringView(key));
}
bool LogEvent::HasContent(const StringView& key) const {
    return contents.find(key) != contents.end();
}
void LogEvent::SetContentNoCopy(const StringView& key, const StringView& val) {
    contents[key] = val;
}

void LogEvent::DelContent(const StringView& key) {
    contents.erase(key);
}

LogEvent::LogEvent() {
    mType = LOG_EVENT_TYPE;
}

} // namespace logtail