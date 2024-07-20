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

#pragma once

#include <dlfcn.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <cstring>
#include <atomic>

#include "ebpf/include/export.h"
#include "ebpf/include/SysAkApi.h"
#include "common/DynamicLibHelper.h"

namespace logtail {
namespace ebpf {

enum class eBPFPluginType {
  SOCKETTRACE = 0,
  PROCESS = 1,
  MAX = 2,
};

class SourceManager {
public:
    const std::string m_lib_name_ = "network_observer";
    
    SourceManager(const SourceManager&) = delete;
    SourceManager& operator=(const SourceManager&) = delete;

    static SourceManager* GetInstance() {
        static SourceManager instance;
        return &instance;
    }

    void Init();

    bool StartPlugin(nami::PluginType plugin_type,
                std::variant<nami::NetworkObserveConfig, nami::ProcessConfig, nami::NetworkSecurityConfig, nami::FileSecurityConfig> config);
    
    bool StopPlugin(nami::PluginType plugin_type);

    bool StopAll();

private:
    void FillCommonConf(nami::eBPFConfig* conf);
    bool LoadDynamicLib(const std::string& lib_name);
    bool DynamicLibSuccess();
    bool UpdatePlugin(nami::PluginType plugin_type, 
                std::variant<nami::NetworkObserveConfig, nami::ProcessConfig, nami::NetworkSecurityConfig, nami::FileSecurityConfig> config);
    
    SourceManager();
    ~SourceManager();

    enum class ebpf_func {
        EBPF_INIT = 0,
        EBPF_UPDATE = 1,
        EBPF_DEINIT = 2,
        EBPF_REMOVE = 3,
        EBPF_SOCKET_TRACE_CLEAN_UP_DOG = 4,
        EBPF_SOCKET_TRACE_UPDATE_CONN_ADDR = 5,
        EBPF_SOCKET_TRACE_DISABLE_PROCESS = 6,
        EBPF_SOCKET_TRACE_UPDATE_CONN_ROLE = 7,
        EBPF_MAX = 8,
    };

    std::shared_ptr<DynamicLibLoader> lib_;
    std::vector<void*> funcs_ = std::vector<void*>((int)ebpf_func::EBPF_MAX, nullptr);
    std::vector<long> offsets_ = std::vector<long>((int)ebpf_func::EBPF_MAX, 0);
    std::vector<std::atomic_bool> running_ = std::vector<std::atomic_bool>(int(nami::PluginType::MAX));
    std::string host_ip_;
    std::string host_name_;
    std::string host_path_prefix_;
    std::string binary_path_;
    std::string full_lib_name_;

#ifdef APSARA_UNIT_TEST_MAIN
    nami::eBPFConfig* config_;
    friend class eBPFServerUnittest;
#endif
};

}
}