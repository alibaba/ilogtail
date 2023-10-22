#pragma once

#include <string>
#include <unordered_map>

#include "json/json.h"

#include "pipeline/PipelineContext.h"

namespace logtail {

struct ContainerFilters {
    std::string mK8sNamespaceRegex;
    std::string mK8sPodRegex;
    std::unordered_map<std::string, std::string> mIncludeK8sLabel;
    std::unordered_map<std::string, std::string> mExcludeK8sLabel;
    std::string mK8sContainerRegex;
    std::unordered_map<std::string, std::string> mIncludeEnv;
    std::unordered_map<std::string, std::string> mExcludeEnv;
    std::unordered_map<std::string, std::string> mIncludeContainerLabel;
    std::unordered_map<std::string, std::string> mExcludeContainerLabel;

    bool Init(const Json::Value& config, const PipelineContext& ctx, const std::string& pluginName);
};

struct ContainerDiscoveryOptions {
    ContainerFilters mContainerFilters;
    std::unordered_map<std::string, std::string> mExternalK8sLabelTag;
    std::unordered_map<std::string, std::string> mExternalEnvTag;
    bool mCollectingContainersMeta = false;

    bool Init(const Json::Value& config, const PipelineContext& ctx, const std::string& pluginName);
};

} // namespace logtail
