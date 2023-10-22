#include "container_manager/ContainerDiscoveryOptions.h"

#include "common/ParamExtractor.h"

using namespace std;

namespace logtail {

bool ContainerFilters::Init(const Json::Value& config, const PipelineContext& ctx, const string& pluginName) {
    string errorMsg;
    // K8pluginNamespaceRegex
    if (!GetOptionalStringParam(config, "ContainerFilters.K8sNamespaceRegex", mK8sNamespaceRegex, errorMsg)) {
        PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    }

    // K8sPodRegex
    if (!GetOptionalStringParam(config, "ContainerFilters.K8sPodRegex", mK8sPodRegex, errorMsg)) {
        PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    }

    // IncludeK8sLabel
    if (!GetOptionalMapParam(config, "ContainerFilters.IncludeK8sLabel", mIncludeK8sLabel, errorMsg)) {
        PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    }

    // ExcludeK8sLabel
    if (!GetOptionalMapParam(config, "ContainerFilters.ExcludeK8sLabel", mExcludeK8sLabel, errorMsg)) {
        PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    }

    // K8sContainerRegex
    if (!GetOptionalStringParam(config, "ContainerFilters.K8sContainerRegex", mK8sContainerRegex, errorMsg)) {
        PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    }

    // IncludeEnv
    if (!GetOptionalMapParam(config, "ContainerFilters.IncludeEnv", mIncludeEnv, errorMsg)) {
        PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    }

    // ExcludeEnv
    if (!GetOptionalMapParam(config, "ContainerFilters.ExcludeEnv", mExcludeEnv, errorMsg)) {
        PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    }

    // IncludeContainerLabel
    if (!GetOptionalMapParam(config, "ContainerFilters.IncludeContainerLabel", mIncludeContainerLabel, errorMsg)) {
        PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    }

    // ExcludeContainerLabel
    if (!GetOptionalMapParam(config, "ContainerFilters.ExcludeContainerLabel", mExcludeContainerLabel, errorMsg)) {
        PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    }

    return true;
}

bool ContainerDiscoveryOptions::Init(const Json::Value& config,
                                     const PipelineContext& ctx,
                                     const string& pluginName) {
    string errorMsg;

    const char* key = "ContainerFilters";
    const Json::Value* itr = config.find(key, key + strlen(key));
    if (itr) {
        if (!itr->isObject()) {
            PARAM_WARNING_IGNORE(
                ctx.GetLogger(), "param ContainerFilters is not of type object", pluginName, ctx.GetConfigName());
        } else {
            mContainerFilters.Init(*itr, ctx, pluginName);
        }
    }

    // ExternalK8sLabelTag
    if (!GetOptionalMapParam(config, "ExternalK8sLabelTag", mExternalK8sLabelTag, errorMsg)) {
        PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    }

    // ExternalEnvTag
    if (!GetOptionalMapParam(config, "ExternalEnvTag", mExternalEnvTag, errorMsg)) {
        PARAM_WARNING_IGNORE(ctx.GetLogger(), errorMsg, pluginName, ctx.GetConfigName());
    }

    // CollectingContainersMeta
    if (!GetOptionalBoolParam(config, "CollectingContainersMeta", mCollectingContainersMeta, errorMsg)) {
        PARAM_WARNING_DEFAULT(ctx.GetLogger(), errorMsg, false, pluginName, ctx.GetConfigName());
    }

    return true;
}

} // namespace logtail
