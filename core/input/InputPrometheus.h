#pragma once

#include "plugin/interface/Input.h"
#include "prometheus/Scraper.h"

namespace logtail {

class InputPrometheus : public Input {
public:
    static const std::string sName;

    InputPrometheus();

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config, Json::Value& optionalGoPipeline) override;
    bool Start() override;
    bool Stop(bool isPipelineRemoving) override;

private:
    // only one job is supported
    // std::vector<ScrapeJob> scrapeJobs;
    ScrapeJob scrapeJob;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class InputPrometheusUnittest;
#endif
};

} // namespace logtail