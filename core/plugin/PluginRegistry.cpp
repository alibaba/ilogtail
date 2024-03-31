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

#include "plugin/PluginRegistry.h"

#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "app_config/AppConfig.h"
#include "flusher/FlusherSLS.h"
#include "input/InputFile.h"
#if defined(__linux__) && !defined(__ANDROID__)
#include "input/InputObserverNetwork.h"
#ifdef __ENTERPRISE__
#include "input/InputStream.h"
#endif
#endif
#include "logger/Logger.h"
#include "plugin/creator/CProcessor.h"
#include "plugin/creator/DynamicCProcessorCreator.h"
#include "plugin/creator/StaticFlusherCreator.h"
#include "plugin/creator/StaticInputCreator.h"
#include "plugin/creator/StaticProcessorCreator.h"
#include "processor/ProcessorDesensitizeNative.h"
#include "processor/ProcessorFilterNative.h"
#include "processor/ProcessorParseApsaraNative.h"
#include "processor/ProcessorParseDelimiterNative.h"
#include "processor/ProcessorParseJsonNative.h"
#include "processor/ProcessorParseRegexNative.h"
#include "processor/ProcessorParseTimestampNative.h"
#include "processor/ProcessorSplitLogStringNative.h"
#include "processor/ProcessorSplitMultilineLogStringNative.h"
#include "processor/ProcessorTagNative.h"
#if defined(__linux__) && !defined(__ANDROID__)
#include "processor/ProcessorSPL.h"
#endif
#include "common/Flags.h"


DEFINE_FLAG_BOOL(enable_processor_spl, "", false);

using namespace std;

namespace logtail {

PluginRegistry::PluginRegistry() {
    mGoPlugins = {"service_canal",
                  "service_docker_event",
                  "service_docker_stdout_raw",
                  "service_docker_stdout",
                  "service_input_example",
                  "service_go_profile",
                  "service_http_server",
                  "service_jmx",
                  "service_kafka",
                  "service_lumberjack",
                  "service_mock",
                  "service_mqtt",
                  "service_mysql",
                  "service_otlp",
                  "service_mssql",
                  "service_pgsql",
                  "service_skywalking_agent_v2",
                  "service_skywalking_agent_v3_http",
                  "service_skywalking_agent_v3",
                  "service_syslog",
                  "service_udp_server",
                  "metric_debug_file",
                  "metric_docker_file",
                  "metric_checkpoint_example",
                  "metric_input_example",
                  "metric_meta_host",
                  "metric_http",
                  "metric_meta_kubernetes",
                  "metric_mock",
                  "metric_binlog",
                  "metric_input_netping",
                  "metric_nginx_status",
                  "metric_process_v2",
                  "metric_redis",
                  "metric_system",
                  "metric_system_v2",
#ifdef _MSC_VER
                  "service_wineventlog",
#else
                  "input_command",
                  "service_gpu_metric",
                  "service_journal",
                  "service_snmp",
                  "service_telegraf",
                  "service_prometheus",
#endif
#ifdef __ENTERPRISE__
                  "service_sunfire_heartbeat",
                  "metric_sunfire_nginx",
                  "metric_sunfire_nginx_status",
                  "metric_sunfire_process",
                  "metric_sunfire_system",
                  "metric_alimetrics_custom",
                  "metric_alimetrics_middleware",
#endif
                  "processor_add_fields",
                  "processor_anchor",
                  "processor_appender",
                  "processor_base64_decoding",
                  "processor_base64_encoding",
                  "processor_cloud_meta",
                  "processor_csv",
                  "processor_default",
                  "processor_desensitize",
                  "processor_dict_map",
                  "processor_drop",
                  "processor_drop_last_key",
                  "processor_encrypt",
                  "processor_fields_with_condition",
                  "processor_filter_key_regex",
                  "processor_filter_regex",
                  "processor_geoip",
                  "processor_gotime",
                  "processor_grok",
                  "processor_json",
                  "processor_log_to_sls_metric",
                  "processor_md5",
                  "processor_otel_metric",
                  "processor_otel_trace",
                  "processor_packjson",
                  "processor_pick_key",
                  "processor_rate_limit",
                  "processor_regex",
                  "processor_rename",
                  "processor_split_char",
                  "processor_split_key_value",
                  "processor_split_log_regex",
                  "processor_split_log_string",
                  "processor_split_string",
                  "processor_string_replace",
                  "processor_strptime",
                  "processor_sql",
#ifdef __ENTERPRISE__
                  "processor_guess_timestamp",
                  "processor_uniquedim",
#endif
                  "aggregator_default",
                  "aggregator_base",
                  "aggregator_content_value_group",
                  "aggregator_context",
                  "aggregator_logstore_router",
                  "aggregator_metadata_group",
                  "aggregator_opentelemetry",
                  "aggregator_shardhash",
                  "aggregator_skywalking",
#ifdef __ENTERPRISE__
                  "aggregator_sunfire",
#endif
                  "flusher_checker",
                  "flusher_http",
                  "flusher_kafka",
                  "flusher_sleep",
                  "flusher_statistics",
                  "flusher_stdout",
#ifdef __ENTERPRISE__
                  "flusher_http_tj",
#else
                  "flusher_clickhouse",
                  "flusher_elasticsearch",
                  "flusher_grpc",
                  "flusher_kafka_v2",
                  "flusher_loki",
                  "flusher_otlp",
                  "flusher_pulsar",
#endif
                  "ext_basicauth",
                  "ext_default_decoder",
                  "ext_groupinfo_filter",
                  "ext_request_breaker"};
}

void PluginRegistry::LoadPlugins() {
    LoadStaticPlugins();
    auto& plugins = AppConfig::GetInstance()->GetDynamicPlugins();
    LoadDynamicPlugins(plugins);
}

void PluginRegistry::UnloadPlugins() {
    // for (auto& kv : mPluginDict) {
    // if (node->plugin_type() == PLUGIN_TYPE_DYNAMIC) {
    //     CPluginRegistryItem* registry = reinterpret_cast<CPluginRegistryItem*>(node);
    //     if (strcmp(registry->mPlugin->language, "Go") == 0) {
    //         destroy_go_plugin_interface(registry->_handle,
    //         const_cast<plugin_interface_t*>(registry->mPlugin));
    //     }
    // }
    //     UnregisterCreator(kv.second.get());
    // }
    mPluginDict.clear();
}

unique_ptr<InputInstance> PluginRegistry::CreateInput(const string& name, const string& pluginId) {
    return unique_ptr<InputInstance>(static_cast<InputInstance*>(Create(INPUT_PLUGIN, name, pluginId).release()));
}

unique_ptr<ProcessorInstance> PluginRegistry::CreateProcessor(const string& name, const string& pluginId) {
    return unique_ptr<ProcessorInstance>(
        static_cast<ProcessorInstance*>(Create(PROCESSOR_PLUGIN, name, pluginId).release()));
}

unique_ptr<FlusherInstance> PluginRegistry::CreateFlusher(const string& name, const string& pluginId) {
    return unique_ptr<FlusherInstance>(static_cast<FlusherInstance*>(Create(FLUSHER_PLUGIN, name, pluginId).release()));
}

bool PluginRegistry::IsValidGoPlugin(const string& name) const {
    return mGoPlugins.find(name) != mGoPlugins.end();
}

bool PluginRegistry::IsValidNativeInputPlugin(const string& name) const {
    return mPluginDict.find(PluginKey(INPUT_PLUGIN, name)) != mPluginDict.end();
}

bool PluginRegistry::IsValidNativeProcessorPlugin(const string& name) const {
    return mPluginDict.find(PluginKey(PROCESSOR_PLUGIN, name)) != mPluginDict.end();
}

bool PluginRegistry::IsValidNativeFlusherPlugin(const string& name) const {
    return mPluginDict.find(PluginKey(FLUSHER_PLUGIN, name)) != mPluginDict.end();
}

void PluginRegistry::LoadStaticPlugins() {
    RegisterInputCreator(new StaticInputCreator<InputFile>());
#if defined(__linux__) && !defined(__ANDROID__)
    RegisterInputCreator(new StaticInputCreator<InputObserverNetwork>());
#ifdef __ENTERPRISE__
    RegisterInputCreator(new StaticInputCreator<InputStream>());
#endif
#endif

    RegisterProcessorCreator(new StaticProcessorCreator<ProcessorSplitLogStringNative>());
    RegisterProcessorCreator(new StaticProcessorCreator<ProcessorSplitMultilineLogStringNative>());
    RegisterProcessorCreator(new StaticProcessorCreator<ProcessorParseApsaraNative>());
    RegisterProcessorCreator(new StaticProcessorCreator<ProcessorParseDelimiterNative>());
    RegisterProcessorCreator(new StaticProcessorCreator<ProcessorDesensitizeNative>());
    RegisterProcessorCreator(new StaticProcessorCreator<ProcessorParseJsonNative>());
    RegisterProcessorCreator(new StaticProcessorCreator<ProcessorParseRegexNative>());
    RegisterProcessorCreator(new StaticProcessorCreator<ProcessorParseTimestampNative>());
    RegisterProcessorCreator(new StaticProcessorCreator<ProcessorTagNative>());
    RegisterProcessorCreator(new StaticProcessorCreator<ProcessorFilterNative>());
#if defined(__linux__) && !defined(__ANDROID__)
    if (BOOL_FLAG(enable_processor_spl)) {
        RegisterProcessorCreator(new StaticProcessorCreator<ProcessorSPL>());
    }
#endif

    RegisterFlusherCreator(new StaticFlusherCreator<FlusherSLS>());
}

void PluginRegistry::LoadDynamicPlugins(const set<string>& plugins) {
    if (plugins.empty()) {
        return;
    }
    string error;
    auto pluginDir = AppConfig::GetInstance()->GetProcessExecutionDir() + "/plugins";
    for (auto& pluginName : plugins) {
        DynamicLibLoader loader;
        if (!loader.LoadDynLib(pluginName, error, pluginDir)) {
            LOG_ERROR(sLogger, ("open plugin", pluginName)("error", error));
            continue;
        }
        PluginCreator* creator = LoadProcessorPlugin(loader, pluginName);
        if (creator) {
            RegisterProcessorCreator(creator);
            continue;
        }
    }
}

void PluginRegistry::RegisterInputCreator(PluginCreator* creator) {
    RegisterCreator(INPUT_PLUGIN, creator);
}

void PluginRegistry::RegisterProcessorCreator(PluginCreator* creator) {
    RegisterCreator(PROCESSOR_PLUGIN, creator);
}

void PluginRegistry::RegisterFlusherCreator(PluginCreator* creator) {
    RegisterCreator(FLUSHER_PLUGIN, creator);
}

PluginCreator* PluginRegistry::LoadProcessorPlugin(DynamicLibLoader& loader, const string pluginName) {
    string error;
    processor_interface_t* plugin = (processor_interface_t*)loader.LoadMethod("processor_interface", error);
    // if (!error.empty()) {
    //     loader.LoadMethod("x_cgo_init", error)
    //     if (error.empty()) { // try go plugin
    //         plugin = create_go_plugin_interface(handle);
    //     }
    // }
    if (!error.empty() || !plugin) {
        LOG_ERROR(sLogger, ("load method", "plugin_interface")("error", error));
        return nullptr;
    }
    if (plugin->version != PROCESSOR_INTERFACE_VERSION) {
        LOG_ERROR(sLogger,
                  ("load plugin", pluginName)("error", "plugin interface version mismatch")(
                      "expected", PROCESSOR_INTERFACE_VERSION)("actual", plugin->version));
        return nullptr;
    }
    return new DynamicCProcessorCreator(plugin, loader.Release());
}

void PluginRegistry::RegisterCreator(PluginCat cat, PluginCreator* creator) {
    if (!creator) {
        return;
    }
    mPluginDict.emplace(PluginKey(cat, creator->Name()), shared_ptr<PluginCreator>(creator));
}

unique_ptr<PluginInstance> PluginRegistry::Create(PluginCat cat, const string& name, const string& pluginId) {
    unique_ptr<PluginInstance> ins;
    auto creatorEntry = mPluginDict.find(PluginKey(cat, name));
    if (creatorEntry != mPluginDict.end()) {
        ins = creatorEntry->second->Create(pluginId);
    }
    return ins;
}

} // namespace logtail
