#pragma once

#include <string>

#include "plugin/interface/Input.h"

namespace logtail {
class InputObserverNetwork : public Input {
public:
    static const std::string sName;
    
    const std::string& Name() const override { return sName; }
    // bool Init(const Table& config) override;
    bool Init(const Json::Value& config, Json::Value& optionalGoPipeline) override;
    bool Start() override;
    bool Stop(bool isPipelineRemoving) override;

private:
    std::string mDetail;
};
} // namespace logtail
