// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
    if (mInited) {
        return;
    }
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
    mInited = true;
}

void eBPFServer::Stop() {
    LOG_INFO(sLogger, ("begin to stop all plugins", ""));
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
    bool ret = false;
    // call update function
    // step2: call init function
    switch(type) {
    case nami::PluginType::PROCESS_SECURITY: {
        nami::ProcessConfig pconfig;
        // pconfig.process_security_cb_ = std::bind(&SecurityHandler::handle, mProcessSecureCB.get(), std::placeholders::_1);
        pconfig.process_security_cb_ = [this](auto events) { return mProcessSecureCB->handle(std::move(events)); };
        SecurityOptions* opts = std::get<SecurityOptions*>(options);
        pconfig.options_ = opts->mOptionList;
        config = std::move(pconfig);
        ret = mSourceManager->StartPlugin(type, config);
        if (ret) mProcessSecureCB->update_context(ctx, plugin_index);
        break;
    }

    case nami::PluginType::NETWORK_OBSERVE:{
        nami::NetworkObserveConfig nconfig;
        // nconfig.measure_cb_ = std::bind(&MeterHandler::handle, mMeterCB.get(), std::placeholders::_1, std::placeholders::_2);
        // nconfig.span_cb_ = std::bind(&SpanHandler::handle, mSpanCB.get(), std::placeholders::_1);
        nconfig.measure_cb_ = [this](auto events, auto ts) { return mMeterCB->handle(std::move(events), ts); };
        nconfig.span_cb_ = [this](auto events) { return mSpanCB->handle(std::move(events)); };
        config = std::move(nconfig);
        ret = mSourceManager->StartPlugin(type, config);
        if (ret) mMeterCB->update_context(ctx, plugin_index);
        if (ret) mSpanCB->update_context(ctx, plugin_index);
        break;
    }

    case nami::PluginType::NETWORK_SECURITY:{
        nami::NetworkSecurityConfig nconfig;
        // nconfig.network_security_cb_ = std::bind(&SecurityHandler::handle, mNetworkSecureCB.get(), std::placeholders::_1);
        nconfig.network_security_cb_ = [this](auto events) { return mNetworkSecureCB->handle(std::move(events)); };
        SecurityOptions* opts = std::get<SecurityOptions*>(options);
        nconfig.options_ = opts->mOptionList;
        config = std::move(nconfig);
        ret = mSourceManager->StartPlugin(type, config);
        if (ret) mNetworkSecureCB->update_context(ctx, plugin_index);
        break;
    }

    case nami::PluginType::FILE_SECURITY:{
        nami::FileSecurityConfig fconfig;
        // fconfig.file_security_cb_ = std::bind(&SecurityHandler::handle, mFileSecureCB.get(), std::placeholders::_1);
        fconfig.file_security_cb_ = [this](auto events) { return mFileSecureCB->handle(std::move(events)); };
        SecurityOptions* opts = std::get<SecurityOptions*>(options);
        fconfig.options_ = opts->mOptionList;
        config = std::move(fconfig);
        ret = mSourceManager->StartPlugin(type, config);
        if (ret) mFileSecureCB->update_context(ctx, plugin_index);
        break;
    }
    default:
        LOG_ERROR(sLogger, ("unknown plugin type", int(type)));
        return false;
    }

    return ret;
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
    // mark plugin status is update
    bool ret = mSourceManager->SuspendPlugin(type);
    if (!ret) return false;
    switch (type) {
    case nami::PluginType::PROCESS_SECURITY:{
        if (mProcessSecureCB) mProcessSecureCB->update_context(nullptr, 0);
        return true;
    }
    case nami::PluginType::NETWORK_OBSERVE:{
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
