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
    if (inited_.load()) {
        return;
    }
    inited_.store(true);
    SourceManager::GetInstance()->Init();
    // ebpf config
    auto configJson = AppConfig::GetInstance()->GetConfig();
    config_.LoadEbpfConfig(configJson);
    ob_meter_cb_ = std::make_unique<ArmsMeterHandler>(nullptr, 0);
    ob_span_cb_ = std::make_unique<ArmsSpanHandler>(nullptr, 0);
    network_secure_cb_ = std::make_unique<SecurityHandler>(nullptr, 0);
    process_secure_cb_ = std::make_unique<SecurityHandler>(nullptr, 0);
    file_secure_cb_ = std::make_unique<SecurityHandler>(nullptr, 0);
}

void eBPFServer::Stop() {
    LOG_WARNING(sLogger, ("begin to stop all plugins", ""));
    SourceManager::GetInstance()->StopAll();
    ob_meter_cb_->update_context(nullptr, 0);
    ob_span_cb_->update_context(nullptr,0);
    network_secure_cb_->update_context(nullptr, 0);
    process_secure_cb_->update_context(nullptr, 0);
    file_secure_cb_->update_context(nullptr, 0);
}

bool eBPFServer::StartPluginInternal(const std::string& pipeline_name, uint32_t plugin_index,
                        nami::PluginType type, 
                        const logtail::PipelineContext* ctx, 
                        const std::variant<SecurityOptions*, ObserverOptions*> options) {

    // step1: convert options to export type
    std::variant<nami::NetworkObserveConfig, nami::ProcessConfig, nami::NetworkSecurityConfig, nami::FileSecurityConfig> config;
    // call update function 
    // step2: call init function
    switch(type) {
    case nami::PluginType::PROCESS: {
        process_secure_cb_->update_context(ctx, plugin_index);
        nami::ProcessConfig pconfig;
        pconfig.process_security_cb_ = std::bind(&SecurityHandler::handle, process_secure_cb_.get(), std::placeholders::_1);
        SecurityOptions* opts = std::get<SecurityOptions*>(options);
        for (auto& item : opts->mOptionList) {
            std::vector<std::string> names = item.mCallName;
            nami::SecurityOptions ak_option;
            ak_option.call_names_ = item.mCallName;
            auto& filter = std::get<SecurityProcessFilter>(item.mFilter);
            nami::SecurityProcessFilter ak_filter;
            ak_filter.mNamespaceFilter = std::vector<nami::SecurityProcessNamespaceFilter>(filter.mNamespaceFilter.size());
            std::transform(filter.mNamespaceFilter.begin(), filter.mNamespaceFilter.end(), ak_filter.mNamespaceFilter.begin(),
                   [](SecurityProcessNamespaceFilter val) -> nami::SecurityProcessNamespaceFilter {
                       nami::SecurityProcessNamespaceFilter tmp;
                       tmp.mValueList = val.mValueList;
                       tmp.mNamespaceType = val.mNamespaceType;
                       return tmp;
                   });
            ak_filter.mNamespaceBlackFilter = std::vector<nami::SecurityProcessNamespaceFilter>(filter.mNamespaceBlackFilter.size());
            std::transform(filter.mNamespaceBlackFilter.begin(), filter.mNamespaceBlackFilter.end(), ak_filter.mNamespaceBlackFilter.begin(),
                   [](SecurityProcessNamespaceFilter val) -> nami::SecurityProcessNamespaceFilter {
                       nami::SecurityProcessNamespaceFilter tmp;
                       tmp.mValueList = val.mValueList;
                       tmp.mNamespaceType = val.mNamespaceType;
                       return tmp;
                   });
            ak_option.filter_ = ak_filter;
            pconfig.options_.emplace_back(std::move(ak_option));
        }

        config = pconfig;
        break;
        // convert
    }
    case nami::PluginType::NETWORK:{
        nami::NetworkObserveConfig nconfig;
        ob_meter_cb_->update_context(ctx, plugin_index);
        ob_span_cb_->update_context(ctx, plugin_index);
        nconfig.measure_cb_ = std::bind(&MeterHandler::handle, ob_meter_cb_.get(), std::placeholders::_1, std::placeholders::_2);
        nconfig.span_cb_ = std::bind(&SpanHandler::handle, ob_span_cb_.get(), std::placeholders::_1);
        // ObserverOptions* opts = std::get<ObserverOptions*>(options);
        config = nconfig;
        break;
    }
    case nami::PluginType::NETWORK_SECURITY:{
        nami::NetworkSecurityConfig nconfig;
        network_secure_cb_->update_context(ctx, plugin_index);
        nconfig.network_security_cb_ = std::bind(&SecurityHandler::handle, network_secure_cb_.get(), std::placeholders::_1);

        SecurityOptions* opts = std::get<SecurityOptions*>(options);

        for (auto& item : opts->mOptionList) {
            std::vector<std::string> names = item.mCallName;
            nami::SecurityOptions ak_option;
            ak_option.call_names_ = item.mCallName;
            nami::SecurityNetworkFilter ak_filter;
            auto& filter = std::get<SecurityNetworkFilter>(item.mFilter);
            ak_filter.mDestAddrList = filter.mDestAddrList;
            ak_filter.mDestAddrBlackList = filter.mDestAddrBlackList;
            ak_filter.mSourceAddrList = filter.mSourceAddrList;
            ak_filter.mSourceAddrBlackList = filter.mSourceAddrBlackList;
            ak_filter.mDestPortList = filter.mDestPortList;
            ak_filter.mDestPortBlackList = filter.mDestPortBlackList;
            ak_filter.mSourcePortList = filter.mSourcePortList;
            ak_filter.mSourcePortBlackList = filter.mSourcePortBlackList;
            ak_option.filter_ = ak_filter;
            nconfig.options_.emplace_back(ak_option);
        }

        config = nconfig;
        break;
    }
    case nami::PluginType::FILE_SECURITY:{
        nami::FileSecurityConfig fconfig;
        file_secure_cb_->update_context(ctx, plugin_index);
        fconfig.file_security_cb_ = std::bind(&SecurityHandler::handle, file_secure_cb_.get(), std::placeholders::_1);

        SecurityOptions* opts = std::get<SecurityOptions*>(options);

        for (auto& item : opts->mOptionList) {
            std::vector<std::string> names = item.mCallName;
            nami::SecurityOptions ak_option;
            ak_option.call_names_ = item.mCallName;
            auto& filter = std::get<SecurityFileFilter>(item.mFilter);
            nami::SecurityFileFilter ak_filter;
            
            ak_filter.mFileFilterItem = std::vector<nami::SecurityFileFilterItem>(filter.mFileFilterItem.size());
            std::transform(filter.mFileFilterItem.begin(), filter.mFileFilterItem.end(), ak_filter.mFileFilterItem.begin(),
                   [](SecurityFileFilterItem val) -> nami::SecurityFileFilterItem {
                       nami::SecurityFileFilterItem tmp;
                       tmp.mFileName = val.mFileName;
                       tmp.mFilePath = val.mFilePath;
                       return tmp;
                   });
            ak_option.filter_ = ak_filter;
            fconfig.options_.emplace_back(std::move(ak_option));
        }

        config = fconfig;
        break;
    }
    default:
        LOG_ERROR(sLogger, ("unknown plugin type", int(type)));
        return false;
    }

    return SourceManager::GetInstance()->StartPlugin(type, config);
}

bool eBPFServer::EnablePlugin(const std::string& pipeline_name, uint32_t plugin_index,
                        nami::PluginType type, 
                        const PipelineContext* ctx, 
                        const std::variant<SecurityOptions*, ObserverOptions*> options) {
    return StartPluginInternal(pipeline_name, plugin_index, type, ctx, options);
}

// bool eBPFServer::UpdatePlugin(const std::string& pipeline_name, uint32_t plugin_index,
//                         nami::PluginType type, 
//                         const PipelineContext* ctx, 
//                         const std::variant<SecurityOptions*, ObserverOptions*> options) {
//     return StartPluginInternal(pipeline_name, plugin_index, type, UpdataType::SECURE_UPDATE_TYPE_CONFIG_CHAGE, ctx, options);
// }

bool eBPFServer::DisablePlugin(const std::string& pipeline_name, nami::PluginType type) {
    return SourceManager::GetInstance()->StopPlugin(type);;
}

bool eBPFServer::SuspendPlugin(const std::string& pipeline_name, nami::PluginType type) {
    LOG_INFO(sLogger, ("receive plugin update", "now suspend for a while"));
    switch (type) {
    case nami::PluginType::PROCESS:{
        if (process_secure_cb_) process_secure_cb_->update_context(nullptr, 0);
        return true;
    }
    case nami::PluginType::NETWORK:{
        if (ob_meter_cb_) ob_meter_cb_->update_context(nullptr, 0);
        if (ob_span_cb_) ob_span_cb_->update_context(nullptr, 0);
        return true;
    }
    case nami::PluginType::NETWORK_SECURITY:{
        if (network_secure_cb_) network_secure_cb_->update_context(nullptr, 0);
        return true;
    }
    case nami::PluginType::FILE_SECURITY:{
        if (file_secure_cb_) file_secure_cb_->update_context(nullptr, 0);
        return true;
    }
    default:
        return false;
    }
}

} // namespace ebpf
}