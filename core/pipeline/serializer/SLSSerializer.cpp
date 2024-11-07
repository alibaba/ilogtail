// Copyright 2024 iLogtail Authors
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

#include "pipeline/serializer/SLSSerializer.h"

#include "common/Flags.h"
#include "constants/TagConstants.h"
#include "common/compression/CompressType.h"
#include "plugin/flusher/sls/FlusherSLS.h"
#include "protobuf/sls/LogGroupSerializer.h"

#include <json/json.h>
#include <array>

DEFINE_FLAG_INT32(max_send_log_group_size, "bytes", 10 * 1024 * 1024);

using namespace std;

namespace logtail {

template <>
bool Serializer<vector<CompressedLogGroup>>::DoSerialize(vector<CompressedLogGroup>&& p,
                                                         std::string& output,
                                                         std::string& errorMsg) {
    auto inputSize = 0;
    for (auto& item : p) {
        inputSize += item.mData.size();
    }
    mInItemsTotal->Add(1);
    mInItemSizeBytes->Add(inputSize);

    auto before = std::chrono::system_clock::now();
    auto res = Serialize(std::move(p), output, errorMsg);
    mTotalProcessMs->Add(std::chrono::system_clock::now() - before);

    if (res) {
        mOutItemsTotal->Add(1);
        mOutItemSizeBytes->Add(output.size());
    } else {
        mDiscardedItemsTotal->Add(1);
        mDiscardedItemSizeBytes->Add(inputSize);
    }
    return res;
}

enum SpanCacheIdx {
    ATTR_KEY_IDX,
    LINK_KEY_IDX,
    EVENT_KEY_IDX,
    START_TS_KEY_IDX,
    END_TS_KEY_IDX,
    DURATION_KEY_IDX,
    IDX_KEY_MAX,
};

bool SLSEventGroupSerializer::Serialize(BatchedEvents&& group, string& res, string& errorMsg) {
    if (group.mEvents.empty()) {
        errorMsg = "empty event group";
        return false;
    }

    PipelineEvent::Type eventType = group.mEvents[0]->GetType();
    if (eventType == PipelineEvent::Type::NONE) {
        // should not happen
        errorMsg = "unsupported event type in event group";
        return false;
    }

    bool enableNs = mFlusher->GetContext().GetGlobalConfig().mEnableTimestampNanosecond;

    // caculate serialized logGroup size first, where some critical results can be cached
    vector<size_t> logSZ(group.mEvents.size());
    vector<pair<string, size_t>> metricEventContentCache(group.mEvents.size());
    vector<array<string, IDX_KEY_MAX>> spanEventContentCache(group.mEvents.size());
    size_t logGroupSZ = 0;
    switch (eventType) {
        case PipelineEvent::Type::LOG:{
            for (size_t i = 0; i < group.mEvents.size(); ++i) {
                const auto& e = group.mEvents[i].Cast<LogEvent>();
                if (e.Empty()) {
                    continue;
                }
                size_t contentSZ = 0;
                for (const auto& kv : e) {
                    contentSZ += GetLogContentSize(kv.first.size(), kv.second.size());
                }
                logGroupSZ += GetLogSize(contentSZ, enableNs && e.GetTimestampNanosecond(), logSZ[i]);
            }
            break;
        }
        case PipelineEvent::Type::METRIC:{
            for (size_t i = 0; i < group.mEvents.size(); ++i) {
                const auto& e = group.mEvents[i].Cast<MetricEvent>();
                if (e.Is<UntypedSingleValue>()) {
                    metricEventContentCache[i].first = to_string(e.GetValue<UntypedSingleValue>()->mValue);
                } else {
                    // should not happen
                    LOG_ERROR(sLogger,
                              ("unexpected error",
                               "invalid metric event type")("config", mFlusher->GetContext().GetConfigName()));
                    continue;
                }
                metricEventContentCache[i].second = GetMetricLabelSize(e);

                size_t contentSZ = 0;
                contentSZ += GetLogContentSize(METRIC_RESERVED_KEY_NAME.size(), e.GetName().size());
                contentSZ
                    += GetLogContentSize(METRIC_RESERVED_KEY_VALUE.size(), metricEventContentCache[i].first.size());
                contentSZ
                    += GetLogContentSize(METRIC_RESERVED_KEY_TIME_NANO.size(), e.GetTimestampNanosecond() ? 19U : 10U);
                contentSZ += GetLogContentSize(METRIC_RESERVED_KEY_LABELS.size(), metricEventContentCache[i].second);
                logGroupSZ += GetLogSize(contentSZ, false, logSZ[i]);
            }
            break;
        }
        case PipelineEvent::Type::SPAN:
            for (size_t i = 0; i < group.mEvents.size(); ++i) {
                const auto& e = group.mEvents[i].Cast<SpanEvent>();
                size_t contentSZ = 0;
                contentSZ += GetLogContentSize(DEFAULT_TRACE_TAG_TRACE_ID.size(), e.GetTraceId().size());
                contentSZ += GetLogContentSize(DEFAULT_TRACE_TAG_SPAN_ID.size(), e.GetSpanId().size());
                contentSZ += GetLogContentSize(DEFAULT_TRACE_TAG_PARENT_ID.size(), e.GetParentSpanId().size());
                contentSZ += GetLogContentSize(DEFAULT_TRACE_TAG_SPAN_NAME.size(), e.GetName().size());
                contentSZ += GetLogContentSize(DEFAULT_TRACE_TAG_SPAN_KIND.size(), e.GetKindString().size());
                contentSZ += GetLogContentSize(DEFAULT_TRACE_TAG_STATUS_CODE.size(), e.GetStatusString().size());
                // 
                // set tags and scope tags
                Json::Value jsonVal;
                for (auto it = e.TagsBegin(); it != e.TagsEnd(); ++it) {
                    jsonVal[it->first.to_string()] = it->second.to_string();
                }
                for (auto it = e.ScopeTagsBegin(); it != e.ScopeTagsEnd(); ++it) {
                    jsonVal[it->first.to_string()] = it->second.to_string();
                }
                Json::StreamWriterBuilder writer;
                std::string attrString = Json::writeString(writer, jsonVal);
                contentSZ += GetLogContentSize(DEFAULT_TRACE_TAG_ATTRIBUTES.size(), attrString.size());
                spanEventContentCache[i][ATTR_KEY_IDX] = std::move(attrString);

                auto linkString = e.SerializeLinksToString();
                contentSZ += GetLogContentSize(DEFAULT_TRACE_TAG_LINKS.size(), linkString.size());
                spanEventContentCache[i][LINK_KEY_IDX] = std::move(linkString);
                auto eventString = e.SerializeEventsToString();
                contentSZ += GetLogContentSize(DEFAULT_TRACE_TAG_EVENTS.size(), eventString.size());
                spanEventContentCache[i][EVENT_KEY_IDX] = std::move(eventString);

                // time related
                auto startTsNs = std::to_string(e.GetStartTimeNs());
                contentSZ += GetLogContentSize(DEFAULT_TRACE_TAG_START_TIME_NANO.size(), startTsNs.size());
                spanEventContentCache[i][START_TS_KEY_IDX] = std::move(startTsNs);
                auto endTsNs = std::to_string(e.GetEndTimeNs());
                contentSZ += GetLogContentSize(DEFAULT_TRACE_TAG_END_TIME_NANO.size(), endTsNs.size());
                spanEventContentCache[i][END_TS_KEY_IDX] = std::move(endTsNs);
                auto durationNs = std::to_string(e.GetEndTimeNs() - e.GetStartTimeNs());
                contentSZ += GetLogContentSize(DEFAULT_TRACE_TAG_DURATION.size(), durationNs.size());
                spanEventContentCache[i][DURATION_KEY_IDX] = std::move(durationNs);
                logGroupSZ += GetLogSize(contentSZ, false, logSZ[i]);
            }
            break;
        default:
            break;
    }
    if (logGroupSZ == 0) {
        errorMsg = "all empty logs";
        return false;
    }

    // loggroup.category is deprecated, no need to set
    for (const auto& tag : group.mTags.mInner) {
        if (tag.first == LOG_RESERVED_KEY_TOPIC || tag.first == LOG_RESERVED_KEY_SOURCE
            || tag.first == LOG_RESERVED_KEY_MACHINE_UUID) {
            logGroupSZ += GetStringSize(tag.second.size());
        } else {
            logGroupSZ += GetLogTagSize(tag.first.size(), tag.second.size());
        }
    }

    if (static_cast<int32_t>(logGroupSZ) > INT32_FLAG(max_send_log_group_size)) {
        errorMsg = "log group exceeds size limit\tgroup size: " + ToString(logGroupSZ)
            + "\tsize limit: " + ToString(INT32_FLAG(max_send_log_group_size));
        return false;
    }

    thread_local LogGroupSerializer serializer;
    serializer.Prepare(logGroupSZ);
    switch (eventType) {
        case PipelineEvent::Type::LOG:
            for (size_t i = 0; i < group.mEvents.size(); ++i) {
                const auto& logEvent = group.mEvents[i].Cast<LogEvent>();
                serializer.StartToAddLog(logSZ[i]);
                serializer.AddLogTime(logEvent.GetTimestamp());
                for (const auto& kv : logEvent) {
                    serializer.AddLogContent(kv.first, kv.second);
                }
                if (enableNs && logEvent.GetTimestampNanosecond()) {
                    serializer.AddLogTimeNs(logEvent.GetTimestampNanosecond().value());
                }
            }
            break;
        case PipelineEvent::Type::METRIC:
            for (size_t i = 0; i < group.mEvents.size(); ++i) {
                const auto& metricEvent = group.mEvents[i].Cast<MetricEvent>();
                if (metricEvent.Is<std::monostate>()) {
                    continue;
                }
                serializer.StartToAddLog(logSZ[i]);
                serializer.AddLogTime(metricEvent.GetTimestamp());
                serializer.AddLogContentMetricLabel(metricEvent, metricEventContentCache[i].second);
                serializer.AddLogContentMetricTimeNano(metricEvent);
                serializer.AddLogContent(METRIC_RESERVED_KEY_VALUE, metricEventContentCache[i].first);
                serializer.AddLogContent(METRIC_RESERVED_KEY_NAME, metricEvent.GetName());
            }
            break;
        case PipelineEvent::Type::SPAN:
            for (size_t i = 0; i < group.mEvents.size(); ++i) {
                const auto& spanEvent = group.mEvents[i].Cast<SpanEvent>();

                serializer.StartToAddLog(logSZ[i]);
                // set trace_id span_id span_kind status etc
                serializer.AddLogContent(DEFAULT_TRACE_TAG_TRACE_ID, spanEvent.GetTraceId());
                serializer.AddLogContent(DEFAULT_TRACE_TAG_SPAN_ID, spanEvent.GetSpanId());
                serializer.AddLogContent(DEFAULT_TRACE_TAG_PARENT_ID, spanEvent.GetParentSpanId());
                // span_name
                serializer.AddLogContent(DEFAULT_TRACE_TAG_SPAN_NAME, spanEvent.GetName());
                // span_kind
                serializer.AddLogContent(DEFAULT_TRACE_TAG_SPAN_KIND, spanEvent.GetKindString());
                // status_code
                serializer.AddLogContent(DEFAULT_TRACE_TAG_STATUS_CODE, spanEvent.GetStatusString());
                
                //// TODO @qianlu.kk enterprise
                // ip
                // auto ipView = spanEvent.GetTag("ip");
                // if (ipView.size()) {
                //     serializer.AddLogContent(DEFAULT_TRACE_TAG_IP, ipView.to_string());
                // }
                // pid
                // auto appIdItr = group.mTags.mInner.find("pid");
                // if (appIdItr != group.mTags.mInner.end()) {
                //     serializer.AddLogContent(DEFAULT_TRACE_TAG_APP_ID, appIdItr->second.to_string());
                // }
                serializer.AddLogContent(DEFAULT_TRACE_TAG_ATTRIBUTES, spanEventContentCache[i][ATTR_KEY_IDX]);
                
                serializer.AddLogTime(spanEvent.GetTimestamp());
                serializer.AddLogContent(DEFAULT_TRACE_TAG_LINKS, spanEventContentCache[i][LINK_KEY_IDX]);
                serializer.AddLogContent(DEFAULT_TRACE_TAG_EVENTS, spanEventContentCache[i][EVENT_KEY_IDX]);

                // start_time
                serializer.AddLogContent(DEFAULT_TRACE_TAG_START_TIME_NANO, spanEventContentCache[i][START_TS_KEY_IDX]);
                // end_time
                serializer.AddLogContent(DEFAULT_TRACE_TAG_END_TIME_NANO, spanEventContentCache[i][END_TS_KEY_IDX]);
                // duration
                serializer.AddLogContent(DEFAULT_TRACE_TAG_DURATION, spanEventContentCache[i][DURATION_KEY_IDX]);
            }
            break;
        default:
            break;
    }
    for (const auto& tag : group.mTags.mInner) {
        if (tag.first == LOG_RESERVED_KEY_TOPIC) {
            serializer.AddTopic(tag.second);
        } else if (tag.first == LOG_RESERVED_KEY_SOURCE) {
            serializer.AddSource(tag.second);
        } else if (tag.first == LOG_RESERVED_KEY_MACHINE_UUID) {
            serializer.AddMachineUUID(tag.second);
        } else {
            serializer.AddLogTag(tag.first, tag.second);
        }
    }
    res = std::move(serializer.GetResult());
    return true;
}

bool SLSEventGroupListSerializer::Serialize(vector<CompressedLogGroup>&& v, string& res, string& errorMsg) {
    sls_logs::SlsLogPackageList logPackageList;
    for (const auto& item : v) {
        auto package = logPackageList.add_packages();
        package->set_data(item.mData);
        package->set_uncompress_size(item.mRawSize);

        CompressType compressType = static_cast<const FlusherSLS*>(mFlusher)->GetCompressType();
        sls_logs::SlsCompressType slsCompressType = sls_logs::SLS_CMP_LZ4;
        if (compressType == CompressType::NONE) {
            slsCompressType = sls_logs::SLS_CMP_NONE;
        } else if (compressType == CompressType::ZSTD) {
            slsCompressType = sls_logs::SLS_CMP_ZSTD;
        }
        package->set_compress_type(slsCompressType);
    }
    res = logPackageList.SerializeAsString();
    return true;
}

} // namespace logtail
