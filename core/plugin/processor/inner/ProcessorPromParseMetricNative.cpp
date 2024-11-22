#include "plugin/processor/inner/ProcessorPromParseMetricNative.h"

#include <json/json.h>

#include "common/StringTools.h"
#include "models/MetricEvent.h"
#include "models/PipelineEventGroup.h"
#include "models/PipelineEventPtr.h"
#include "models/RawEvent.h"
#include "prometheus/Constants.h"
#include "prometheus/Utils.h"

using namespace std;
namespace logtail {

const string ProcessorPromParseMetricNative::sName = "processor_prom_parse_metric_native";

// only for inner processor
bool ProcessorPromParseMetricNative::Init(const Json::Value& config) {
    mScrapeConfigPtr = std::make_unique<ScrapeConfig>();
    if (!mScrapeConfigPtr->InitStaticConfig(config)) {
        return false;
    }
    return true;
}

void ProcessorPromParseMetricNative::Process(PipelineEventGroup& eGroup) {
    if (eGroup.GetMetadata(EventGroupMetaKey::PROMETHEUS_STREAM_ID).empty()) {
        LOG_WARNING(sLogger, ("unexpected event", "need prom stream id"));
        return;
    }

    EventsContainer& events = eGroup.MutableEvents();
    auto rawEvents = std::move(events);

    StringView scrapeTimestampMilliSecStr = eGroup.GetMetadata(EventGroupMetaKey::PROMETHEUS_SCRAPE_TIMESTAMP_MILLISEC);
    auto timestampMilliSec = StringTo<uint64_t>(scrapeTimestampMilliSecStr.to_string());
    auto timestamp = timestampMilliSec / 1000;
    auto nanoSec = timestampMilliSec % 1000 * 1000000;
    TextParser parser(mScrapeConfigPtr->mHonorTimestamps);
    parser.SetDefaultTimestamp(timestamp, nanoSec);

    for (auto& rawEvent : rawEvents) {
        ProcessEvent(rawEvent, events, eGroup, parser);
    }

    auto streamID = eGroup.GetMetadata(EventGroupMetaKey::PROMETHEUS_STREAM_ID).to_string();

    // cache the metrics count
    {
        auto lock = std::lock_guard<std::mutex>(mCountMutex);
        mMetricCountCache[streamID] += events.size();
        // if up is set, then add self monitor metrics
        if (!eGroup.GetMetadata(EventGroupMetaKey::PROMETHEUS_UP_STATE).empty()) {
            eGroup.SetMetadata(EventGroupMetaKey::PROMETHEUS_SAMPLES_SCRAPED, ToString(mMetricCountCache[streamID]));
            // erase the cache
            mMetricCountCache.erase(streamID);
        }
    }

    if (!eGroup.GetMetadata(EventGroupMetaKey::PROMETHEUS_UP_STATE).empty()) {
        // erase the cache
        std::lock_guard<std::mutex> lock(mRawDataMutex);
        mMetricRawCache.erase(streamID);
    }
}

bool ProcessorPromParseMetricNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<RawEvent>();
}

bool ProcessorPromParseMetricNative::ProcessEvent(PipelineEventPtr& e,
                                                  EventsContainer& events,
                                                  PipelineEventGroup& eGroup,
                                                  TextParser& parser) {
    if (!e.Is<RawEvent>()) {
        LOG_WARNING(sLogger, ("unexpected event type", "need raw event"));
        return false;
    }
    auto streamID = eGroup.GetMetadata(EventGroupMetaKey::PROMETHEUS_STREAM_ID).to_string();
    auto data = e.Cast<RawEvent>().GetContent();
    string cache;
    {
        std::lock_guard<std::mutex> lock(mRawDataMutex);
        cache = mMetricRawCache[streamID];
        mMetricRawCache[streamID] = "";
    }

    size_t begin = 0;
    for (size_t end = begin; end < data.size(); ++end) {
        if (data[end] == '\n') {
            if (begin == 0 && !cache.empty()) {
                cache.append(data.data(), end);
                auto sb = eGroup.GetSourceBuffer()->CopyString(cache.data(), cache.size());
                AddEvent(sb.data, sb.size, events, eGroup, parser);
            } else if (begin != end) {
                AddEvent(data.data() + begin, end - begin, events, eGroup, parser);
            }
            begin = end + 1;
        }
    }
    if (begin < data.size()) {
        std::lock_guard<std::mutex> lock(mRawDataMutex);
        mMetricRawCache[streamID].append(data.data() + begin, data.size() - begin);
    }

    return true;
}

void ProcessorPromParseMetricNative::AddEvent(
    const char* data, size_t size, EventsContainer& events, PipelineEventGroup& eGroup, TextParser& parser) {
    auto metricEvent = eGroup.CreateMetricEvent(true);
    auto line = StringView(data, size);
    if (IsValidMetric(line) && parser.ParseLine(line, *metricEvent)) {
        metricEvent->SetTag(string(prometheus::NAME), metricEvent->GetName());
        events.emplace_back(std::move(metricEvent), true, nullptr);
    }
}

} // namespace logtail
