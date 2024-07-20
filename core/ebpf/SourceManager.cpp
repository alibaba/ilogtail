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

#include "ebpf/SourceManager.h"
#include "common/RuntimeUtil.h"
#include "logger/Logger.h"
#include "ebpf/include/SysAkApi.h"
#include "common/MachineInfoUtil.h"
#include "common/LogtailCommonFlags.h"

#include <string>
#include <filesystem>
#include <memory>
#include <gflags/gflags.h>

namespace logtail{
namespace ebpf {

#define LOAD_EBPF_FUNC_ADDR(funcName) \
    ({ \
        void* funcPtr = tmp_lib->LoadMethod(#funcName, loadErr); \
        if (funcPtr == NULL) { \
            LOG_ERROR(sLogger, ("[source_manager] load ebpf method", "failed")("method", #funcName)("error", loadErr)); \
        } \
        funcPtr; \
    })

#define LOAD_UPROBE_OFFSET_FAIL (-100)
#define LOAD_UPROBE_OFFSET(funcName) \
    ({ \
        Dl_info dlinfo; \
        int dlAddrErr = dladdr((const void*)(funcName), &dlinfo); \
        long res = 0; \
        if (dlAddrErr == 0) { \
            LOG_ERROR(sLogger, \
                      ("[source_manager] load ebpf dl address", "failed")("error", dlAddrErr)("method", #funcName)); \
            res = LOAD_UPROBE_OFFSET_FAIL; \
        } else { \
            res = (long)dlinfo.dli_saddr - (long)dlinfo.dli_fbase; \
            LOG_DEBUG(sLogger, ("func", #funcName)("offset", res)); \
        } \
        res; \
    })
    
SourceManager::SourceManager() {
}

SourceManager::~SourceManager() {
}

void SourceManager::Init() {
  host_ip_ = GetHostIp();
  host_name_ = GetHostName();
  host_path_prefix_ = STRING_FLAG(default_container_host_path);
  binary_path_ = GetProcessExecutionDir();
  full_lib_name_ = "lib" + m_lib_name_ + ".so";
}

bool SourceManager::LoadDynamicLib(const std::string& lib_name) {
  if (DynamicLibSuccess()) {
    // already load
    return true;
  }
#ifdef APSARA_UNIT_TEST_MAIN
    return true;
#endif
  std::shared_ptr<DynamicLibLoader> tmp_lib = std::make_shared<DynamicLibLoader>();
  LOG_INFO(sLogger, ("[SourceManager] begin load ebpf dylib, path:", binary_path_));
  std::string loadErr;
  if (!tmp_lib->LoadDynLib(lib_name, loadErr, binary_path_)) {
      LOG_ERROR(sLogger, ("failed to load ebpf dynamic library, path", binary_path_)("error", loadErr));
      return false;
  }
  // init funcs
  // set offset configs
  // init config
  // this memory will be managed by plugin

  funcs_[(int)ebpf_func::EBPF_INIT] = LOAD_EBPF_FUNC_ADDR(init);
  funcs_[(int)ebpf_func::EBPF_UPDATE] = LOAD_EBPF_FUNC_ADDR(update);
  funcs_[(int)ebpf_func::EBPF_DEINIT] = LOAD_EBPF_FUNC_ADDR(deinit);
  funcs_[(int)ebpf_func::EBPF_REMOVE] = LOAD_EBPF_FUNC_ADDR(removep);
  funcs_[(int)ebpf_func::EBPF_SOCKET_TRACE_CLEAN_UP_DOG] = LOAD_EBPF_FUNC_ADDR(ebpf_cleanup_dog);
  funcs_[(int)ebpf_func::EBPF_SOCKET_TRACE_UPDATE_CONN_ROLE] = LOAD_EBPF_FUNC_ADDR(ebpf_update_conn_role);
  funcs_[(int)ebpf_func::EBPF_SOCKET_TRACE_DISABLE_PROCESS] = LOAD_EBPF_FUNC_ADDR(ebpf_disable_process);
  funcs_[(int)ebpf_func::EBPF_SOCKET_TRACE_UPDATE_CONN_ADDR] = LOAD_EBPF_FUNC_ADDR(ebpf_update_conn_addr);

  offsets_[(int)ebpf_func::EBPF_INIT] = LOAD_UPROBE_OFFSET(funcs_[(int)ebpf_func::EBPF_INIT]);
  offsets_[(int)ebpf_func::EBPF_UPDATE] = LOAD_UPROBE_OFFSET(funcs_[(int)ebpf_func::EBPF_UPDATE]);
  offsets_[(int)ebpf_func::EBPF_DEINIT] = LOAD_UPROBE_OFFSET(funcs_[(int)ebpf_func::EBPF_DEINIT]);
  offsets_[(int)ebpf_func::EBPF_REMOVE] = LOAD_UPROBE_OFFSET(funcs_[(int)ebpf_func::EBPF_REMOVE]);
  offsets_[(int)ebpf_func::EBPF_SOCKET_TRACE_CLEAN_UP_DOG] = LOAD_UPROBE_OFFSET(funcs_[(int)ebpf_func::EBPF_SOCKET_TRACE_CLEAN_UP_DOG]);
  offsets_[(int)ebpf_func::EBPF_SOCKET_TRACE_UPDATE_CONN_ROLE] = LOAD_UPROBE_OFFSET(funcs_[(int)ebpf_func::EBPF_SOCKET_TRACE_UPDATE_CONN_ROLE]);
  offsets_[(int)ebpf_func::EBPF_SOCKET_TRACE_DISABLE_PROCESS] = LOAD_UPROBE_OFFSET(funcs_[(int)ebpf_func::EBPF_SOCKET_TRACE_DISABLE_PROCESS]);
  offsets_[(int)ebpf_func::EBPF_SOCKET_TRACE_UPDATE_CONN_ADDR] = LOAD_UPROBE_OFFSET(funcs_[(int)ebpf_func::EBPF_SOCKET_TRACE_UPDATE_CONN_ADDR]);

  // check function load success
  for (auto& x : funcs_) {
    if (x == nullptr) return false;
  }

  // update meta
  lib_ = std::move(tmp_lib);

  return true;
}

bool SourceManager::DynamicLibSuccess() {
#ifdef APSARA_UNIT_TEST_MAIN
    return true;
#endif
  if (!lib_) return false;
  for (auto x : funcs_) {
    if (x == nullptr) return false;
  }
  return true;
}

void SourceManager::FillCommonConf(nami::eBPFConfig* conf) {
  conf->host_ip_ = host_ip_;
  conf->host_name_ = host_name_;
  conf->host_path_prefix_ = host_path_prefix_;
  if (conf->plugin_type_ == nami::PluginType::NETWORK) {
    auto cc = std::get<nami::NetworkObserveConfig>(conf->config_);
    // set so addr
    cc.so_ = std::filesystem::path(binary_path_) / full_lib_name_;
    cc.so_size_ = cc.so_.length();
    cc.uprobe_offset_ = offsets_[(int)ebpf_func::EBPF_SOCKET_TRACE_CLEAN_UP_DOG];
    cc.upcr_offset_ = offsets_[(int)ebpf_func::EBPF_SOCKET_TRACE_UPDATE_CONN_ROLE];
    cc.upps_offset_ = offsets_[(int)ebpf_func::EBPF_SOCKET_TRACE_DISABLE_PROCESS];
    cc.upca_offset_ = offsets_[(int)ebpf_func::EBPF_SOCKET_TRACE_UPDATE_CONN_ADDR];
    conf->config_ = cc;
  }
}

bool SourceManager::StartPlugin(nami::PluginType plugin_type, 
                std::variant<nami::NetworkObserveConfig, nami::ProcessConfig, nami::NetworkSecurityConfig, nami::FileSecurityConfig> config) {
  if (!LoadDynamicLib(m_lib_name_)) {
    LOG_ERROR(sLogger, ("failed to load dynamic lib", "exit"));
    return false;
  }

  if (running_[int(plugin_type)]) {
    // do update
    return UpdatePlugin(plugin_type, std::move(config));
  }

  LOG_INFO(sLogger, ("begin to start plugin, type", int(plugin_type)));

  auto update_f = (init_func)funcs_[(int)ebpf_func::EBPF_INIT];
  auto conf = new nami::eBPFConfig;
  conf->plugin_type_ = plugin_type;
  conf->type = UpdataType::SECURE_UPDATE_TYPE_ENABLE_PROBE;
  conf->config_ = config;
  FillCommonConf(conf);
#ifdef APSARA_UNIT_TEST_MAIN
    config_ = conf;
    running_[int(plugin_type)] = true;
    return true;
#endif
  int res = update_f(conf);
  if (res) {
    return false;
  } else {
    running_[int(plugin_type)] = true;
    return true;
  }
}

bool SourceManager::UpdatePlugin(nami::PluginType plugin_type, 
                std::variant<nami::NetworkObserveConfig, nami::ProcessConfig, nami::NetworkSecurityConfig, nami::FileSecurityConfig> config) {
  if (!LoadDynamicLib(m_lib_name_)) {
    LOG_ERROR(sLogger, ("failed to load dynamic lib", "exit"));
    return false;
  }

  if (!running_[int(plugin_type)]) {
    // check update
    LOG_ERROR(sLogger, ("plugin not started, type",  int(plugin_type)));
    return false;
  }

  auto update_f = (update_func)funcs_[(int)ebpf_func::EBPF_UPDATE];
  auto conf = new nami::eBPFConfig;
  conf->plugin_type_ = plugin_type;
  conf->type = UpdataType::SECURE_UPDATE_TYPE_CONFIG_CHAGE;
  conf->config_ = config;
  FillCommonConf(conf);
#ifdef APSARA_UNIT_TEST_MAIN
  config_ = conf;
  return true;
#endif
  int res = update_f(conf);
  return res ? false : true;
}

bool SourceManager::StopAll() {
  if (!DynamicLibSuccess()) {
    LOG_WARNING(sLogger, ("dynamic lib not load, just exit", "need check"));
    return true;
  }

  for (size_t i = 0; i < running_.size(); i ++) {
    auto& x = running_[i];
    if (!x) continue;
    // stop plugin
    StopPlugin(static_cast<nami::PluginType>(i));
  }

#ifdef APSARA_UNIT_TEST_MAIN
  return true;
#endif

  // call deinit
  auto deinit_f = (deinit_func)funcs_[(int)ebpf_func::EBPF_DEINIT];
  deinit_f();
  return true;
}

bool SourceManager::StopPlugin(nami::PluginType plugin_type) {
  if (!DynamicLibSuccess()) {
    LOG_WARNING(sLogger, ("dynamic lib not load, just exit", "need check"));
    return true;
  }

  if (!running_[int(plugin_type)]) {
    LOG_WARNING(sLogger, ("plugin already stopped, type",  int(plugin_type)));
    return true;
  }

  auto remove_f = (remove_func)funcs_[(int)ebpf_func::EBPF_REMOVE];
  auto config = new nami::eBPFConfig;
  config->plugin_type_ = plugin_type;
  config->type = UpdataType::SECURE_UPDATE_TYPE_DISABLE_PROBE;

#ifdef APSARA_UNIT_TEST_MAIN
  config_ = config;
  running_[int(plugin_type)] = false;
  return true;
#endif
  int res = remove_f(config);
  if (res) {
    return false;
  } else {
    running_[int(plugin_type)] = false;
    return true;
  }
}

}
}