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

#include "ebpf/security/SecurityServer.h"
#include "queue/ProcessQueueManager.h"
#include "queue/ProcessQueueItem.h"
#include "common/MachineInfoUtil.h"
#include "common/LogtailCommonFlags.h"
#include "logger/Logger.h"
#include "ebpf/include/ProcessApi.h"
#include "models/LogEvent.h"
#include "models/PipelineEventGroup.h"
#include "models/PipelineEvent.h"
#include "queue/ProcessQueueManager.h"
#include "queue/ProcessQueueItem.h"

#include <thread>
#include <mutex>
#include <iostream>
#include <memory>
#include <chrono>
#include <functional>

DEFINE_FLAG_BOOL(secure_cb_use_class_method, "whether use class method as callback or not, default is true", true);
DEFINE_FLAG_BOOL(secure_enable_libbpf_debug, "whether enable libbpf debug for processsecure or not, default is true", false);
DEFINE_FLAG_BOOL(secure_enable_ebpf_feature, "whether enable secure ebpf feature or not, default is false", false);


namespace logtail {

// 负责接收ebpf返回的数据，然后将数据推送到对应的队列中
// TODO: 目前暂时没有考虑并发Start的问题

// input 代码 + 联调安全
void SecurityServer::Start(BPFSecurityPipelineType type) {
    if (mIsRunning) {
        return;
    } else {
        // TODO: 创建一个线程，用于接收ebpf返回的数据，并将数据推送到对应的队列中
        LOG_INFO(sLogger, ("security ebpf server", "started"));
    }
}

void SecurityServer::Stop(BPFSecurityPipelineType type) {
    // TODO: ebpf_stop(); 停止所有类型的ebpf探针
}

void SecurityServer::Stop() {
    logtail::ebpf::SourceManager::GetInstance()->StopProcessProbe();
}

// 插件配置注册逻辑
// 负责启动对应的ebpf程序
void SecurityServer::AddSecurityOptions(const std::string& name,
                                        size_t index,
                                        const SecurityOptions* options,
                                        const PipelineContext* ctx) {
    std::string key = name + "#" + std::to_string(index);
    mInputConfigMap[key] = std::make_pair(options, ctx);
    // TODO: 目前一种类型的input只能处理一个，后续需要修改
    switch (options->mFilterType) {
        case SecurityFilterType::FILE: {
            // TODO: ebpf_start(type);
            file_config_ = std::make_pair(options, ctx);
            break;
        }
        case SecurityFilterType::PROCESS: {
            // TODO: ebpf_start(type);
            process_config_ = std::make_pair(options, ctx);
            break;
        }
        case SecurityFilterType::NETWORK: {
            // TODO: ebpf_start(type);
            network_config_ = std::make_pair(options, ctx);
            break;
        }
        default:
            break;
    }
}
// 插件配置注销逻辑
// TODO: 目前处理配置变更，先stop掉该类型的探针，然后在map里remove配置
void SecurityServer::RemoveSecurityOptions(const std::string& name, size_t index) {
    std::string key = name + "#" + std::to_string(index);
    // TODO: 目前一种类型的input只能处理一个，后续需要修改
    switch (mInputConfigMap[key].first->mFilterType) {
        case SecurityFilterType::FILE: {
            // TODO: ebpf_stop(type);
            StopBPF(BPFSecurityPipelineType::PIPELINE_FILE);
            file_config_ = std::make_pair(nullptr, nullptr);
            break;
        }
        case SecurityFilterType::PROCESS: {
            // TODO: ebpf_stop(type);
            StopBPF(BPFSecurityPipelineType::PIPELINE_PROCESS);
            process_config_ = std::make_pair(nullptr, nullptr);
            break;
        }
        case SecurityFilterType::NETWORK: {
            // TODO: ebpf_stop(type);
            StopBPF(BPFSecurityPipelineType::PIPELINE_NETWORK);
            network_config_ = std::make_pair(nullptr, nullptr);
            break;
        }
        default:
            break;
    }
    mInputConfigMap.erase(key);
}

void SecurityServer::Init() {}

void HandleSecureEvent(std::unique_ptr<AbstractSecurityEvent> event) {
    LOG_DEBUG(sLogger, ("HandleSecureEvent", "enter"));
    SecurityServer::GetInstance()->HandleProcessSecureEvent(std::move(event));
    LOG_DEBUG(sLogger, ("HandleSecureEvent", "exit"));
    return;
}

void HandleBatchSecureEvent(std::vector<std::unique_ptr<AbstractSecurityEvent>> events) {
    LOG_DEBUG(sLogger, ("HandleBatchSecureEvent", "enter"));
    SecurityServer::GetInstance()->HandleBatchProcessSecureEvents(std::move(events));
    LOG_DEBUG(sLogger, ("HandleBatchSecureEvent", "exit"));
}

void SecurityServer::HandleBatchProcessSecureEvents(std::vector<std::unique_ptr<AbstractSecurityEvent>>&& events) {
    LOG_DEBUG(sLogger, ("HandleBatchProcessSecureEvents enter, event size", events.size()));
    HandleBatchEventsInternel(this->process_config_.second, std::move(events));
    LOG_DEBUG(sLogger, ("HandleBatchProcessSecureEvents", "exit"));
}
void SecurityServer::HandleBatchNetworSecureEvents(std::vector<std::unique_ptr<AbstractSecurityEvent>>&& events) {
    LOG_DEBUG(sLogger, ("HandleBatchNetworSecureEvents enter, event size", events.size()));
    HandleBatchEventsInternel(this->network_config_.second, std::move(events));
    LOG_DEBUG(sLogger, ("HandleBatchNetworSecureEvents", "exit"));
}

void SecurityServer::HandleBatchEventsInternel(const PipelineContext* ctx, std::vector<std::unique_ptr<AbstractSecurityEvent>>&& events) {
    if (ctx == nullptr || events.empty()) {
        return ;
    }
    std::shared_ptr<SourceBuffer> mSourceBuffer = std::make_shared<SourceBuffer>();;
    PipelineEventGroup mTestEventGroup(mSourceBuffer);
    // aggregate to pipeline event group
    // set host ips
    mTestEventGroup.SetTag("host_ip", host_ip_);
    mTestEventGroup.SetTag("host_name", host_name_);
    for (auto& x : events) {
        auto event = mTestEventGroup.AddLogEvent();
        for (auto& tag : x->GetAllTags()) {
            event->SetContent(tag.first, tag.second);
        }

        time_t ts = static_cast<time_t>(x->GetTimestamp() / 1e9);
        event->SetTimestamp(ts);
    }
    std::unique_ptr<ProcessQueueItem> item = 
            std::unique_ptr<ProcessQueueItem>(new ProcessQueueItem(std::move(mTestEventGroup), 0));
    
    if (ProcessQueueManager::GetInstance()->PushQueue(ctx->GetProcessQueueKey(), std::move(item))) {
        LOG_WARNING(sLogger, ("Push queue failed!", events.size()));
    } else {
        LOG_INFO(sLogger, ("Push queue success!", events.size()));
    }
    return;
}

void SecurityServer::HandleProcessSecureEvent(std::unique_ptr<AbstractSecurityEvent>&& event) {
    LOG_DEBUG(sLogger, ("HandleSecureEvent", "enter"));
    if (event == nullptr) {return;}
    // TODO @qianlu.kk merge multi events into a group
    auto ctx = this->process_config_.second;
    if (ctx == nullptr) {
        LOG_ERROR(sLogger, ("HandleSecureEvent ctx", "null"));
        return;
    }
    auto source_buffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup group(source_buffer);
    auto log_event = group.AddLogEvent();
    auto tags = event->GetAllTags();
    for (auto tag : tags) {
        log_event->SetContent(tag.first, tag.second);
    }

    std::unique_ptr<ProcessQueueItem> item = 
            std::unique_ptr<ProcessQueueItem>(new ProcessQueueItem(std::move(group), 0));
    ProcessQueueManager::GetInstance()->PushQueue(ctx->GetProcessQueueKey(), std::move(item));
    LOG_DEBUG(sLogger, ("Push queue", "done"));
    return;
}

void SecurityServer::SetConfig(SecureConfig* config_, BPFSecurityPipelineType type) {
    config_->host_path_prefix_ = STRING_FLAG(default_container_host_path);
    /// get host name
    this->host_name_ = GetHostName();
    config_->host_name_ = host_name_;
    /// get host ip
    this->host_ip_ = GetHostIp();
    config_->host_ip_ = host_ip_;
    config_->enable_libbpf_debug_ = BOOL_FLAG(secure_enable_libbpf_debug);

    if (type == BPFSecurityPipelineType::PIPELINE_NETWORK) {
        config_->plugin_type = SecureEventType::SECURE_EVENT_TYPE_SOCKET_SECURE;
        if (FLAGS_secure_cb_use_class_method) {
            config_->net_batch_cb_ = std::bind(&SecurityServer::HandleBatchNetworSecureEvents, this, std::placeholders::_1);
            config_->net_single_cb_ = nullptr;
        } else {
            config_->net_batch_cb_ = nullptr;
            config_->net_single_cb_ = nullptr;
        }
        config_->enable_probes_[(int)SecureEventType::SECURE_EVENT_TYPE_SOCKET_SECURE] = true;
        auto& options = network_config_.first;
        for (auto& item : options->mOptionList) {
            std::vector<std::string> names = item.mCallName;
            config_->network_call_names_ = names;
            auto& filter = std::get<SecurityNetworkFilter>(item.mFilter);
            config_->enable_dips_ = filter.mDestAddrList;
            config_->disable_dips_ = filter.mDestAddrBlackList;
            config_->enable_sips_ = filter.mSourceAddrList;
            config_->disable_sips_ = filter.mSourceAddrBlackList;
            config_->enable_dports_ = filter.mDestPortList;
            config_->disable_dports_ = filter.mDestPortBlackList;
            config_->enable_sports_ = filter.mSourcePortList;
            config_->disable_sports_ = filter.mSourcePortBlackList;
        }
    } else if (type == BPFSecurityPipelineType::PIPELINE_PROCESS) {
        config_->plugin_type = SecureEventType::SECURE_EVENT_TYPE_PROCESS_SECURE;
        if (FLAGS_secure_cb_use_class_method) {
            config_->proc_single_cb_ = std::bind(&SecurityServer::HandleProcessSecureEvent, this, std::placeholders::_1);
            config_->proc_batch_cb_ = std::bind(&SecurityServer::HandleBatchProcessSecureEvents, this, std::placeholders::_1);
        } else {
            config_->proc_single_cb_ = HandleSecureEvent;
            config_->proc_batch_cb_ = HandleBatchSecureEvent;
        }
        config_->enable_probes_[(int)SecureEventType::SECURE_EVENT_TYPE_PROCESS_SECURE] = true;
        auto& options = process_config_.first;
        for (auto& item : options->mOptionList) {
            std::vector<std::string> names = item.mCallName;
            config_->network_call_names_ = names;
            auto& filter = std::get<SecurityProcessFilter>(item.mFilter);
            config_->enable_namespaces_ = std::vector<NamespaceFilter>(filter.mNamespaceFilter.size());
            std::transform(filter.mNamespaceFilter.begin(), filter.mNamespaceFilter.end(), config_->enable_namespaces_.begin(),
                   [](SecurityProcessNamespaceFilter val) -> NamespaceFilter {
                       NamespaceFilter tmp;
                       tmp.mValueList = val.mValueList;
                       tmp.mNamespaceType = val.mNamespaceType;
                       return tmp;
                   });
            config_->disable_namespaces_ = std::vector<NamespaceFilter>(filter.mNamespaceBlackFilter.size());
            std::transform(filter.mNamespaceBlackFilter.begin(), filter.mNamespaceBlackFilter.end(), config_->disable_namespaces_.begin(),
                   [](SecurityProcessNamespaceFilter val) -> NamespaceFilter {
                       NamespaceFilter tmp;
                       tmp.mValueList = val.mValueList;
                       tmp.mNamespaceType = val.mNamespaceType;
                       return tmp;
                   });
        }
    } else {
        // config_->enable_probes_[(int)SecureEventType::SECURE_EVENT_TYPE_MAX];
    }
}

bool SecurityServer::UpdateBPFConfig(BPFSecurityPipelineType type) {
    SecureConfig* config_ = new SecureConfig;
    config_->type = UpdataType::SECURE_UPDATE_TYPE_CONFIG_CHAGE;
    SetConfig(config_, type);
    // ensure dl running
    bool res = logtail::ebpf::SourceManager::GetInstance()->LoadAndStartDynamicLib(ebpf::eBPFPluginType::PROCESS, (void*)config_);
    if (!res) {
        LOG_INFO(sLogger, ("begin to call update config for plugin type", (int)config_->plugin_type));
        return false;
    }
    // config may changed, force update it ...
    LOG_INFO(sLogger, ("begin to call update config for plugin type", (int)config_->plugin_type));
    return logtail::ebpf::SourceManager::GetInstance()->UpdateConfig(ebpf::eBPFPluginType::PROCESS, (void*)config_);
}

void SecurityServer::InitBPF(BPFSecurityPipelineType type) {
    IncreaseRef();
    // init configs
    SecureConfig* config_ = new SecureConfig;
    config_->type = UpdataType::SECURE_UPDATE_TYPE_ENABLE_PROBE;
    config_->host_path_prefix_ = STRING_FLAG(default_container_host_path);
    /// get host name
    this->host_name_ = GetHostName();
    config_->host_name_ = host_name_;
    /// get host ip
    this->host_ip_ = GetHostIp();
    config_->host_ip_ = host_ip_;
    config_->enable_libbpf_debug_ = BOOL_FLAG(secure_enable_libbpf_debug);
    /// set callbacks
    if (type == BPFSecurityPipelineType::PIPELINE_NETWORK) {
        config_->plugin_type = SecureEventType::SECURE_EVENT_TYPE_SOCKET_SECURE;
        if (FLAGS_secure_cb_use_class_method) {
            config_->net_batch_cb_ = std::bind(&SecurityServer::HandleBatchNetworSecureEvents, this, std::placeholders::_1);
            config_->net_single_cb_ = nullptr;
        } else {
            config_->net_batch_cb_ = nullptr;
            config_->net_single_cb_ = nullptr;
        }
        config_->enable_probes_[(int)SecureEventType::SECURE_EVENT_TYPE_SOCKET_SECURE] = true;
        auto& options = network_config_.first;
        for (auto& item : options->mOptionList) {
            std::vector<std::string> names = item.mCallName;
            config_->network_call_names_ = names;
            auto& filter = std::get<SecurityNetworkFilter>(item.mFilter);
            config_->enable_dips_ = filter.mDestAddrList;
            config_->disable_dips_ = filter.mDestAddrBlackList;
            config_->enable_sips_ = filter.mSourceAddrList;
            config_->disable_sips_ = filter.mSourceAddrBlackList;
            config_->enable_dports_ = filter.mDestPortList;
            config_->disable_dports_ = filter.mDestPortBlackList;
            config_->enable_sports_ = filter.mSourcePortList;
            config_->disable_sports_ = filter.mSourcePortBlackList;
        }

    } else if (type == BPFSecurityPipelineType::PIPELINE_PROCESS) {
        config_->plugin_type = SecureEventType::SECURE_EVENT_TYPE_PROCESS_SECURE;
        if (FLAGS_secure_cb_use_class_method) {
            config_->proc_single_cb_ = std::bind(&SecurityServer::HandleProcessSecureEvent, this, std::placeholders::_1);
            config_->proc_batch_cb_ = std::bind(&SecurityServer::HandleBatchProcessSecureEvents, this, std::placeholders::_1);
        } else {
            config_->proc_single_cb_ = HandleSecureEvent;
            config_->proc_batch_cb_ = HandleBatchSecureEvent;
        }
        config_->enable_probes_[(int)SecureEventType::SECURE_EVENT_TYPE_PROCESS_SECURE] = true;
        auto& options = process_config_.first;
        for (auto& item : options->mOptionList) {
            std::vector<std::string> names = item.mCallName;
            config_->network_call_names_ = names;
            auto& filter = std::get<SecurityProcessFilter>(item.mFilter);
            config_->enable_namespaces_ = std::vector<NamespaceFilter>(filter.mNamespaceFilter.size());
            std::transform(filter.mNamespaceFilter.begin(), filter.mNamespaceFilter.end(), config_->enable_namespaces_.begin(),
                   [](SecurityProcessNamespaceFilter val) -> NamespaceFilter {
                       NamespaceFilter tmp;
                       tmp.mValueList = val.mValueList;
                       tmp.mNamespaceType = val.mNamespaceType;
                       return tmp;
                   });
            config_->disable_namespaces_ = std::vector<NamespaceFilter>(filter.mNamespaceBlackFilter.size());
            std::transform(filter.mNamespaceBlackFilter.begin(), filter.mNamespaceBlackFilter.end(), config_->disable_namespaces_.begin(),
                   [](SecurityProcessNamespaceFilter val) -> NamespaceFilter {
                       NamespaceFilter tmp;
                       tmp.mValueList = val.mValueList;
                       tmp.mNamespaceType = val.mNamespaceType;
                       return tmp;
                   });
        }
    } else {
        // config_->enable_probes_[(int)SecureEventType::SECURE_EVENT_TYPE_MAX];
    }
    
    // ensure dl running
    bool res = logtail::ebpf::SourceManager::GetInstance()->LoadAndStartDynamicLib(ebpf::eBPFPluginType::PROCESS, (void*)config_);
    if (!res) return;
    // config may changed, force update it ...
    LOG_INFO(sLogger, ("begin to call update config for plugin type", (int)config_->plugin_type));
    logtail::ebpf::SourceManager::GetInstance()->UpdateConfig(ebpf::eBPFPluginType::PROCESS, (void*)config_);
}

void SecurityServer::StopBPF(BPFSecurityPipelineType type) {
    DecreaseRef();
    if (ref_.load() <= 0) {
        logtail::ebpf::SourceManager::GetInstance()->StopProcessProbe();
        return;
    }
    auto config = new SecureConfig;
    config->type = UpdataType::SECURE_UPDATE_TYPE_DISABLE_PROBE;
    // update config
    if (type == BPFSecurityPipelineType::PIPELINE_FILE) {
        config->plugin_type = SecureEventType::SECURE_EVENT_TYPE_FILE_SECURE;
        // TODO @qianlu.kk
    } else if (type == BPFSecurityPipelineType::PIPELINE_PROCESS) {
        config->plugin_type = SecureEventType::SECURE_EVENT_TYPE_PROCESS_SECURE;
        config->disable_probes_[(int)SecureEventType::SECURE_EVENT_TYPE_PROCESS_SECURE] = true;
        ebpf::SourceManager::GetInstance()->UpdateConfig(ebpf::eBPFPluginType::PROCESS, (void*)config);
    } else if (type == BPFSecurityPipelineType::PIPELINE_NETWORK) {
        config->plugin_type = SecureEventType::SECURE_EVENT_TYPE_SOCKET_SECURE;
        config->disable_probes_[(int)SecureEventType::SECURE_EVENT_TYPE_SOCKET_SECURE] = true;
        ebpf::SourceManager::GetInstance()->UpdateConfig(ebpf::eBPFPluginType::PROCESS, (void*)config);
    }
}

void SecurityServer::IncreaseRef() {
    ref_.fetch_add(1);
}

void SecurityServer::DecreaseRef() {
    ref_.fetch_add(-1);
}

} // namespace logtail