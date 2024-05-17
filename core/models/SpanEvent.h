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

#include <map>
#include <string>
#include <vector>

#include "common/memory/SourceBuffer.h"
#include "models/PipelineEvent.h"

namespace logtail {

// SpanEvent follows the definition of Span in otlp, except for the followings:
// 1. Field SchemaURL and all DroppedxxxCount are ignored
// 2. Attributes value only supports string
// 3. Field InstrumentedScope in ScopeSpan is stored in mScopeTags
// Besides, PipelineEventGroup is equivalent to ResourceSpan in otlp, with Resource Attributes stored in mTags
class SpanEvent : public PipelineEvent {
    friend class PipelineEventGroup;

public:
    class SpanLink {
        friend class SpanEvent;

    public:
        StringView GetTraceId() const { return mTraceId; }
        void SetTraceId(const std::string& traceId);
        void SetTraceIdNoCopy(StringView traceId) { mTraceId = traceId; }

        StringView GetSpanId() const { return mSpanId; }
        void SetSpanId(const std::string& spanId);
        void SetSpanIdNoCopy(StringView spanId) { mSpanId = spanId; }

        StringView GetTraceState() const { return mTraceState; }
        void SetTraceState(const std::string& traceState);
        void SetTraceStateNoCopy(StringView traceState) { mTraceState = traceState; }

        StringView GetTag(StringView key) const;
        bool HasTag(StringView key) const;
        void SetTag(StringView key, StringView val);
        void SetTag(const std::string& key, const std::string& val);
        void SetTagNoCopy(const StringBuffer& key, const StringBuffer& val);
        void SetTagNoCopy(StringView key, StringView val);
        void DelTag(StringView key);

        std::shared_ptr<SourceBuffer>& GetSourceBuffer();

#ifdef APSARA_UNIT_TEST_MAIN
        Json::Value ToJson() const;
        void FromJson(const Json::Value& value);
#endif

    private:
        SpanLink(SpanEvent* parent) : mParent(parent) {}

        StringView mTraceId;
        StringView mSpanId;
        StringView mTraceState;
        std::map<StringView, StringView> mTags;
        SpanEvent* mParent = nullptr;
    };

    class InnerEvent {
        friend class SpanEvent;

    public:
        uint64_t GetTimestampNs() const { return mTimestampNs; }
        void SetTimestampNs(uint64_t timeStampNs) { mTimestampNs = timeStampNs; }

        StringView GetName() const { return mName; }
        void SetName(const std::string& name);

        StringView GetTag(StringView key) const;
        bool HasTag(StringView key) const;
        void SetTag(StringView key, StringView val);
        void SetTag(const std::string& key, const std::string& val);
        void SetTagNoCopy(const StringBuffer& key, const StringBuffer& val);
        void SetTagNoCopy(StringView key, StringView val);
        void DelTag(StringView key);

        std::shared_ptr<SourceBuffer>& GetSourceBuffer();

#ifdef APSARA_UNIT_TEST_MAIN
        Json::Value ToJson() const;
        void FromJson(const Json::Value& value);
#endif

    private:
        InnerEvent(SpanEvent* parent) : mParent(parent) {}

        uint64_t mTimestampNs = 0;
        StringView mName;
        std::map<StringView, StringView> mTags;
        SpanEvent* mParent = nullptr;
    };

    enum class Kind { Unspecified, Internal, Server, Client, Producer, Consumer };
    enum class StatusCode { Unset, Ok, Error };

    static const std::string OTLP_SCOPE_NAME;
    static const std::string OTLP_SCOPE_VERSION;

    StringView GetTraceId() const { return mTraceId; }
    void SetTraceId(const std::string& traceId);

    StringView GetSpanId() const { return mSpanId; }
    void SetSpanId(const std::string& spanId);

    StringView GetTraceState() const { return mTraceState; }
    void SetTraceState(const std::string& traceState);

    StringView GetParentSpanId() const { return mParentSpanId; }
    void SetParentSpanId(const std::string& parentSpanId);

    StringView GetName() const { return mName; }
    void SetName(const std::string& name);

    Kind GetKind() const { return mKind; }
    void SetKind(Kind kind) { mKind = kind; }

    uint64_t GetStartTimeNs() const { return mStartTimeNs; }
    void SetStartTimeNs(uint64_t startTimeNs) { mStartTimeNs = startTimeNs; }

    uint64_t GetEndTimeNs() const { return mEndTimeNs; }
    void SetEndTimeNs(uint64_t endTimeNs) { mEndTimeNs = endTimeNs; }

    StringView GetTag(StringView key) const;
    bool HasTag(StringView key) const;
    void SetTag(StringView key, StringView val);
    void SetTag(const std::string& key, const std::string& val);
    void SetTagNoCopy(const StringBuffer& key, const StringBuffer& val);
    void SetTagNoCopy(StringView key, StringView val);
    void DelTag(StringView key);

    const std::vector<InnerEvent>& GetEvents() const { return mEvents; }
    InnerEvent* AddEvent();

    const std::vector<SpanLink>& GetLinks() const { return mLinks; }
    SpanLink* AddLink();

    StatusCode GetStatus() const { return mStatus; }
    void SetStatus(StatusCode status) { mStatus = status; }

    StringView GetScopeTag(StringView key) const;
    bool HasScopeTag(StringView key) const;
    void SetScopeTag(StringView key, StringView val);
    void SetScopeTag(const std::string& key, const std::string& val);
    void SetScopeTagNoCopy(const StringBuffer& key, const StringBuffer& val);
    void SetScopeTagNoCopy(StringView key, StringView val);
    void DelScopeTag(StringView key);

    uint64_t EventsSizeBytes() override;

#ifdef APSARA_UNIT_TEST_MAIN
    Json::Value ToJson(bool enableEventMeta = false) const override;
    bool FromJson(const Json::Value&) override;
#endif

private:
    SpanEvent(PipelineEventGroup* ptr);

    StringView mTraceId; // required
    StringView mSpanId; // required
    StringView mTraceState;
    StringView mParentSpanId;
    StringView mName; // required
    Kind mKind = Kind::Unspecified;
    uint64_t mStartTimeNs = 0; // required
    uint64_t mEndTimeNs = 0; // required
    std::map<StringView, StringView> mTags;
    std::vector<InnerEvent> mEvents;
    std::vector<SpanLink> mLinks;
    StatusCode mStatus = StatusCode::Unset;
    std::map<StringView, StringView> mScopeTags; // store InstrumentedScope info in otlp
};

} // namespace logtail
