#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <gflags/gflags.h>

#include "app_config/AppConfig.h"
#include "ebpf/config.h"
#include "ebpf/eBPFServer.h"
#include "logger/Logger.h"
#include "ebpf/include/export.h"
#include "common/LogtailCommonFlags.h"

namespace logtail {
namespace ebpf {

void eBPFServer::Init() {
    if (mInited.load()) {
        return;
    }
    mInited.store(true);
    mSourceManager = std::make_unique<SourceManager>();
    mSourceManager->Init();
    // ebpf config
    auto configJson = AppConfig::GetInstance()->GetConfig();
    mAdminConfig.LoadEbpfConfig(configJson);
#ifdef __ENTERPRISE__
    mMeterCB = std::make_unique<ArmsMeterHandler>(nullptr, 0);
    mSpanCB = std::make_unique<ArmsSpanHandler>(nullptr, 0);
#else
    mMeterCB = std::make_unique<OtelMeterHandler>(nullptr, 0);
    mSpanCB = std::make_unique<OtelSpanHandler>(nullptr, 0);
#endif

    mNetworkSecureCB = std::make_unique<SecurityHandler>(nullptr, 0);
    mProcessSecureCB = std::make_unique<SecurityHandler>(nullptr, 0);
    mFileSecureCB = std::make_unique<SecurityHandler>(nullptr, 0);
}

void eBPFServer::Stop() {
    LOG_WARNING(sLogger, ("begin to stop all plugins", ""));
    mSourceManager->StopAll();
    if (mMeterCB) mMeterCB->update_context(nullptr, 0);
    if (mSpanCB) mSpanCB->update_context(nullptr,0);
    if (mNetworkSecureCB) mNetworkSecureCB->update_context(nullptr, 0);
    if (mProcessSecureCB) mProcessSecureCB->update_context(nullptr, 0);
    if (mFileSecureCB) mFileSecureCB->update_context(nullptr, 0);
}

bool eBPFServer::StartPluginInternal(const std::string& pipeline_name, uint32_t plugin_index,
                        nami::PluginType type, 
                        const logtail::PipelineContext* ctx, 
                        const std::variant<SecurityOptions*, nami::ObserverProcessOption*, nami::ObserverFileOption*, nami::ObserverNetworkOption*> options) {

    // step1: convert options to export type
    std::variant<nami::NetworkObserveConfig, nami::ProcessConfig, nami::NetworkSecurityConfig, nami::FileSecurityConfig> config;
    // call update function
    // step2: call init function
    switch(type) {
    case nami::PluginType::PROCESS_SECURITY: {
        mProcessSecureCB->update_context(ctx, plugin_index);
        nami::ProcessConfig pconfig;
        pconfig.process_security_cb_ = std::bind(&SecurityHandler::handle, mProcessSecureCB.get(), std::placeholders::_1);
        SecurityOptions* opts = std::get<SecurityOptions*>(options);
        pconfig.options_ = opts->mOptionList;
        config = std::move(pconfig);
        break;
    }

    case nami::PluginType::NETWORK:{
        nami::NetworkObserveConfig nconfig;
        // nami::ObserverNetworkOption* opts = std::get<nami::ObserverNetworkOption*>(options);
        mMeterCB->update_context(ctx, plugin_index);
        mSpanCB->update_context(ctx, plugin_index);
        nconfig.measure_cb_ = std::bind(&MeterHandler::handle, mMeterCB.get(), std::placeholders::_1, std::placeholders::_2);
        nconfig.span_cb_ = std::bind(&SpanHandler::handle, mSpanCB.get(), std::placeholders::_1);
        config = std::move(nconfig);
        break;
    }

    case nami::PluginType::NETWORK_SECURITY:{
        nami::NetworkSecurityConfig nconfig;
        mNetworkSecureCB->update_context(ctx, plugin_index);
        nconfig.network_security_cb_ = std::bind(&SecurityHandler::handle, mNetworkSecureCB.get(), std::placeholders::_1);
        SecurityOptions* opts = std::get<SecurityOptions*>(options);
        nconfig.options_ = opts->mOptionList;
        config = std::move(nconfig);
        break;
    }

    case nami::PluginType::FILE_SECURITY:{
        nami::FileSecurityConfig fconfig;
        mFileSecureCB->update_context(ctx, plugin_index);
        fconfig.file_security_cb_ = std::bind(&SecurityHandler::handle, mFileSecureCB.get(), std::placeholders::_1);
        SecurityOptions* opts = std::get<SecurityOptions*>(options);
        fconfig.options_ = opts->mOptionList;
        config = std::move(fconfig);
        break;
    }
    default:
        LOG_ERROR(sLogger, ("unknown plugin type", int(type)));
        return false;
    }

    return mSourceManager->StartPlugin(type, config);
}

bool eBPFServer::EnablePlugin(const std::string& pipeline_name, uint32_t plugin_index,
                        nami::PluginType type, 
                        const PipelineContext* ctx, 
                        const std::variant<SecurityOptions*, nami::ObserverProcessOption*, nami::ObserverFileOption*, nami::ObserverNetworkOption*> options) {
    return StartPluginInternal(pipeline_name, plugin_index, type, ctx, options);
}

bool eBPFServer::DisablePlugin(const std::string& pipeline_name, nami::PluginType type) {
    return mSourceManager->StopPlugin(type);;
}

bool eBPFServer::SuspendPlugin(const std::string& pipeline_name, nami::PluginType type) {
    LOG_INFO(sLogger, ("receive plugin update", "now suspend for a while"));
    switch (type) {
    case nami::PluginType::PROCESS_SECURITY:{
        if (mProcessSecureCB) mProcessSecureCB->update_context(nullptr, 0);
        return true;
    }
    case nami::PluginType::NETWORK:{
        if (mMeterCB) mMeterCB->update_context(nullptr, 0);
        if (mSpanCB) mSpanCB->update_context(nullptr, 0);
        return true;
    }
    case nami::PluginType::NETWORK_SECURITY:{
        if (mNetworkSecureCB) mNetworkSecureCB->update_context(nullptr, 0);
        return true;
    }
    case nami::PluginType::FILE_SECURITY:{
        if (mFileSecureCB) mFileSecureCB->update_context(nullptr, 0);
        return true;
    }
    default:
        return false;
    }
}

} // namespace ebpf
}