// Copyright 2026 iLogtail Authors
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

#pragma once

#include <cstdint>

#include <memory>
#include <string>

#include "agentsight.h"
#include "common/StringView.h"
#include "common/memory/SourceBuffer.h"
#include "ebpf/include/export.h"
#include "ebpf/type/CommonDataEvent.h"

namespace logtail::ebpf {

/// First-chunk size of the per-record arena. 1 KiB (kDefaultNodeSourceBufferSize, the same size
/// AggTree/StaticDataRow nodes use) covers a record's small identity/metadata strings in one
/// block; large JSON payloads take SourceBuffer's direct-allocation path (one heap block each,
/// equivalent to the previous per-field std::string malloc). The default 4 KiB would inflate
/// every queued record — the common-event queue holds up to ~8192 of them.
inline constexpr uint32_t kAgentsightRecordChunkSize = 1024;

/// One LLM record handed over by the AgentSight FFI.
///
/// Construction (on the AgentSight poller thread, inside the FFI callback) copies every FFI
/// string exactly once into mSourceBuffer and keeps StringView members aliasing it; the FFI
/// pointers die when the callback returns and are never retained. HandleLlmEvent then hands the
/// arena to the PipelineEventGroup as its main SourceBuffer, so emitted log contents alias these
/// bytes with no second copy (SetContentNoCopy) and the arena outlives the record until flush.
class AgentsightLlmRecord : public CommonEvent {
public:
    AgentsightLlmRecord(std::string pipelineConfigName, const AgentsightLLMData& d);
    // StringView members over a shared arena make a shallow copy a lifetime hazard; records are
    // only ever handled through shared_ptr<CommonEvent> (constructed in place, never moved).
    // Destructor is virtual through CommonEvent.
    ~AgentsightLlmRecord() override = default;
    AgentsightLlmRecord(const AgentsightLlmRecord&) = delete;
    AgentsightLlmRecord& operator=(const AgentsightLlmRecord&) = delete;
    AgentsightLlmRecord(AgentsightLlmRecord&&) = delete;
    AgentsightLlmRecord& operator=(AgentsightLlmRecord&&) = delete;

    PluginType GetPluginType() const override { return PluginType::AGENTSIGHT_OBSERVE; }

    const std::string& GetPipelineConfigName() const { return mPipelineConfigName; }

    /// Arena backing every StringView member. Valid for the record's lifetime; after HandleLlmEvent
    /// the event group co-owns it, keeping the bytes alive until the group is flushed.
    const std::shared_ptr<SourceBuffer>& GetSourceBuffer() const { return mSourceBuffer; }

    std::string mPipelineConfigName;
    std::shared_ptr<SourceBuffer> mSourceBuffer;

    StringView mSessionId{kEmptyStringView};
    StringView mConversationId{kEmptyStringView};
    StringView mResponseId{kEmptyStringView};
    int32_t mPid = 0;
    uint64_t mTimestampNs = 0;
    uint64_t mDurationNs = 0;
    uint16_t mStatusCode = 0;
    uint8_t mIsSse = 0;
    bool mLlmUsage = false;
    uint32_t mInputTokens = 0;
    uint32_t mOutputTokens = 0;
    uint32_t mTotalTokens = 0;
    uint32_t mCacheCreationInputTokens = 0;
    uint32_t mCacheReadInputTokens = 0;
    StringView mProcessName{kEmptyStringView};
    // Space-joined process command line (argv), truncated to 127 bytes; empty when the
    // process has already exited. Maps to AgentsightLLMData.cmdline (char[128]).
    StringView mCmdline{kEmptyStringView};
    StringView mAgentType{kEmptyStringView};
    // Container id resolved from the pid on the agentsight side; empty when the process is
    // not in a container (or resolution failed). Maps to AgentsightLLMData.container_id.
    StringView mContainerId{kEmptyStringView};
    StringView mRequestUrl{kEmptyStringView};
    StringView mProvider{kEmptyStringView};
    StringView mModel{kEmptyStringView};
    StringView mFinishReason{kEmptyStringView};
    // NOTE: AgentsightLLMData.input_message_delta is deliberately NOT retained — the delta is
    // recomputed locally (ComputeInputMessagesDelta) and nothing ever read the FFI copy; keeping
    // it would pin one of the largest JSON blobs per record until the group is flushed.
    StringView mRequestMessagesJson{kEmptyStringView};
    StringView mResponseMessagesJson{kEmptyStringView};
    StringView mToolDefinitionsJson{kEmptyStringView};
};

/// Raw HTTP exchange reported when AgentSight could not parse the traffic into LLM semantics
/// (unknown API path or unrecognised body shape). Requires `RawHttpsFallback: true` plus
/// libagentsight >= 0.9.0; see AgentsightManager::OnHttpsCallback.
///
/// Memory model matches AgentsightLlmRecord: one arena copy at construction (FFI callback),
/// StringView members aliasing it, arena handed to the PipelineEventGroup in HandleHttpsEvent.
///
/// Emitted as an `http.request` / `http.response` pair sharing one `http.exchange.id` (the pairing
/// key; each log keeps its own unique `event.id`, and `event.sequence` orders the two). The response
/// half is omitted only when the response side is genuinely empty (no body and no headers) — not
/// merely when mStatusCode == 0, which HTTP/2 also reports for a fully-populated response whose
/// `:status` pseudo-header could not be HPACK-decoded. See HandleHttpsEvent.
///
/// Narrower than AgentsightLlmRecord, but not by as much as it used to be: process attribution
/// (cmdline / agent type / container id) is now carried too, resolved on the agentsight side from
/// the pid with the same ladder the LLM path uses. Still absent are session id and conversation id —
/// a raw event is by definition traffic that could not be mapped onto LLM semantics, so it has no
/// session to belong to. There is no request_url either — only method + path; the target host is
/// inside mRequestHeaders.
class AgentsightHttpsRecord : public CommonEvent {
public:
    AgentsightHttpsRecord(std::string pipelineConfigName, const AgentsightHttpsData& d);
    // Same arena-ownership rule-of-five rationale as AgentsightLlmRecord.
    ~AgentsightHttpsRecord() override = default;
    AgentsightHttpsRecord(const AgentsightHttpsRecord&) = delete;
    AgentsightHttpsRecord& operator=(const AgentsightHttpsRecord&) = delete;
    AgentsightHttpsRecord(AgentsightHttpsRecord&&) = delete;
    AgentsightHttpsRecord& operator=(AgentsightHttpsRecord&&) = delete;

    PluginType GetPluginType() const override { return PluginType::AGENTSIGHT_OBSERVE; }

    const std::string& GetPipelineConfigName() const { return mPipelineConfigName; }

    const std::shared_ptr<SourceBuffer>& GetSourceBuffer() const { return mSourceBuffer; }

    std::string mPipelineConfigName;
    std::shared_ptr<SourceBuffer> mSourceBuffer;

    int32_t mPid = 0;
    StringView mProcessName{kEmptyStringView};
    // Same semantics as the identically-named AgentsightLlmRecord members: cmdline is space-joined
    // argv truncated to 127 bytes (empty once the process exits), mContainerId is empty outside a
    // container, mAgentType is the config-matched agent name (lowercased by agentsight) and falls
    // back to the process comm when no rule matches.
    //
    // mAgentType is emitted as `agent.type`, NOT `gen_ai.agent.type` — a raw event has no GenAI
    // semantics, so it emits nothing in that namespace. Same value, different key; see
    // FillAgentsightHttpCommon.
    StringView mCmdline{kEmptyStringView};
    StringView mAgentType{kEmptyStringView};
    StringView mContainerId{kEmptyStringView};
    uint64_t mTimestampNs = 0;
    uint64_t mDurationNs = 0;
    StringView mMethod{kEmptyStringView};
    StringView mPath{kEmptyStringView};
    uint16_t mStatusCode = 0;
    uint8_t mIsSse = 0;
    // Length-delimited payloads: AgentsightHttpsData reports these as (ptr, len) pairs and they may
    // contain embedded NULs (compressed or binary bodies), so they are copied by length into the
    // record's arena, not by NUL scan, and may not be valid UTF-8. Consumers must stay
    // length-bounded (StringView / rapidjson two-argument Parse).
    //
    // Filtered through an ALLOWLIST (kAllowedRequestHeaders) before anything is emitted, because this
    // is where credentials live — Authorization / x-api-key / cookie — and this path does no
    // redaction. Survivors go out as `http.request.header.<name>`; host and user-agent are salvaged
    // into `server.address` / `server.port` / `user_agent.original` instead. Anything unrecognised is
    // dropped from the output. See ExtractRequestHeaderFields / FillAgentsightHttpRequestLog.
    // Note: filtered-out bytes are still present in the arena until the event group is flushed
    // (zero-copy trade-off) — they are never referenced by any emitted view.
    StringView mRequestHeaders{kEmptyStringView};
    StringView mRequestBody{kEmptyStringView};
    // Mirror image: filtered through a DENYLIST (kSensitiveResponseHeaders — set-cookie,
    // www-authenticate, ...) and otherwise emitted as `http.response.header.<name>`. An allowlist here
    // would hide most of what makes this fallback diagnostically useful, and the credential-bearing
    // response header names are a short, known set. See EmitHttpResponseHeaders.
    StringView mResponseHeaders{kEmptyStringView};
    StringView mResponseBody{kEmptyStringView};
};

} // namespace logtail::ebpf
