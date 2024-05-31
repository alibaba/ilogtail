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

    // sample
    std::vector<ScrapeJob> scrapeJobs;

private:
};

} // namespace logtail