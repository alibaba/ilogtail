#include "plugin/processor/inner/ProcessorPromParseMetricNative.h"

#include <json/json.h>

#include "common/StringTools.h"
#include "models/MetricEvent.h"
#include "models/PipelineEventGroup.h"
#include "models/PipelineEventPtr.h"
#include "models/RawEvent.h"
#include "prometheus/Constants.h"

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
    EventsContainer& events = eGroup.MutableEvents();
    if (events.size() != 1) {
        LOG_WARNING(sLogger, ("unexpected event size", events.size()));
        return;
    }
    PipelineEventPtr e = std::move(events.front());
    events.pop_back();
    if (!e.Is<RawEvent>()) {
        LOG_WARNING(sLogger, ("unexpected event type", "need raw event"));
        return;
    }
    if (eGroup.GetMetadata(EventGroupMetaKey::PROMETHEUS_STREAM_ID).empty()) {
        LOG_WARNING(sLogger, ("unexpected event", "need prom stream id"));
        return;
    }

    StringView scrapeTimestampMilliSecStr = eGroup.GetMetadata(EventGroupMetaKey::PROMETHEUS_SCRAPE_TIMESTAMP_MILLISEC);
    auto timestampMilliSec = StringTo<uint64_t>(scrapeTimestampMilliSecStr.to_string());
    auto timestamp = timestampMilliSec / 1000;
    auto nanoSec = timestampMilliSec % 1000 * 1000000;
    TextParser parser(mScrapeConfigPtr->mHonorTimestamps);
    parser.SetDefaultTimestamp(timestamp, nanoSec);

    ProcessEvent(e, events, eGroup, parser);


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
}

bool ProcessorPromParseMetricNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<RawEvent>();
}

bool ProcessorPromParseMetricNative::ProcessEvent(PipelineEventPtr& e,
                                                  EventsContainer& events,
                                                  PipelineEventGroup& eGroup,
                                                  TextParser& parser) {
    auto streamID = eGroup.GetMetadata(EventGroupMetaKey::PROMETHEUS_STREAM_ID).to_string();
    auto data = e.Cast<RawEvent>().GetContent();
    string cache;
    {
        std::lock_guard<std::mutex> lock(mRawDataMutex);
        cache = std::move(mMetricRawCache[streamID]);
    }

    size_t begin = 0;
    for (size_t end = begin; end < data.size(); ++end) {
        if (data[end] == '\n') {
            if (begin == 0 && !cache.empty()) {
                cache.append(data.data(), end);
                AddEvent(cache.data(), cache.size(), events, eGroup, parser);
                cache.clear();
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
    if (!eGroup.GetMetadata(EventGroupMetaKey::PROMETHEUS_UP_STATE).empty()) {
        // erase the cache
        std::lock_guard<std::mutex> lock(mRawDataMutex);
        mMetricRawCache.erase(streamID);
    }
    return true;
}

void ProcessorPromParseMetricNative::AddEvent(
    const char* data, size_t size, EventsContainer& events, PipelineEventGroup& eGroup, TextParser& parser) {
    auto metricEvent = eGroup.CreateMetricEvent(true);
    if (parser.ParseLine(StringView(data, size), *metricEvent)) {
        metricEvent->SetTag(string(prometheus::NAME), metricEvent->GetName());
        events.emplace_back(std::move(metricEvent), true, nullptr);
    }
}

} // namespace logtail
