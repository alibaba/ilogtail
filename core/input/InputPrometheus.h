#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "plugin/interface/Input.h"
#include "prometheus/ScrapeJob.h"

namespace logtail {

class InputPrometheus : public Input {
public:
    static const std::string sName;

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config, uint32_t& pluginIdx, Json::Value& optionalGoPipeline) override;
    bool Start() override;
    bool Stop(bool isPipelineRemoving) override;

    std::string mJobName;

private:
    bool CreateInnerProcessors(const Json::Value& inputConfig, uint32_t& pluginIdx);
    // only one job is supported
    std::unique_ptr<ScrapeJob> mScrapeJobPtr;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class InputPrometheusUnittest;
#endif
};

} // namespace logtail