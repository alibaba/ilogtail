#pragma once

#include <string>
#include <unordered_map>

#include "models/PipelineEventGroup.h"
#include "models/PipelineEventPtr.h"
#include "pipeline/plugin/interface/Processor.h"
#include "prometheus/labels/TextParser.h"
#include "prometheus/schedulers/ScrapeConfig.h"

DECLARE_FLAG_INT32(process_thread_count);

namespace logtail {
class ProcessorPromParseMetricNative : public Processor {
public:
    static const std::string sName;

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config) override;
    void Process(PipelineEventGroup&) override;

protected:
    bool IsSupportedEvent(const PipelineEventPtr&) const override;

private:
    bool ProcessEvent(PipelineEventPtr&, EventsContainer&, PipelineEventGroup&, TextParser& parser);
    void
    AddEvent(const char* data, size_t size, EventsContainer& events, PipelineEventGroup& eGroup, TextParser& parser);

    void Lock() {
        if (INT32_FLAG(process_thread_count) > 1) {
            mStreamMutex.lock();
        }
    }
    void UnLock() {
        if (INT32_FLAG(process_thread_count) > 1) {
            mStreamMutex.unlock();
        }
    }
    std::unique_ptr<ScrapeConfig> mScrapeConfigPtr;

    std::mutex mStreamMutex;
    std::unordered_map<std::string, int64_t> mStreamTotalCache;
    std::unordered_map<std::string, int64_t> mStreamCountCache;
    std::unordered_map<std::string, int64_t> mMetricCountCache;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class InputPrometheusUnittest;
#endif
};

} // namespace logtail
