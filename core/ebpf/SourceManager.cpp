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
    
SourceManager::SourceManager() : 
        m_libs_(std::vector<std::shared_ptr<DynamicLibLoader>>(static_cast<int>(eBPFPluginType::MAX), nullptr)),
        running_(std::vector<std::atomic_bool>(static_cast<int>(eBPFPluginType::MAX))) {
}

SourceManager::~SourceManager() {
    // remove libs
}

bool SourceManager::UpdateConfig(eBPFPluginType type, void* config) {
  if (!m_libs_[static_cast<int>(type)] || !DynamicLibLoadSucess(type)) {
    return false;
  }
  switch (type)
  {
  case eBPFPluginType::SOCKETTRACE: {
    auto it = lib_funcs_.find(static_cast<int>(eBPFPluginType::SOCKETTRACE));
    if (it != lib_funcs_.end()) {
      auto funcs = it->second;
      auto f = (update_func)funcs[socket_trace_func::SOCKET_TRACE_UPDATE];
      f(config);
      return true;
    } else {
      return false;
    }
    break;
  }
  case eBPFPluginType::PROCESS:{
    auto it = lib_funcs_.find(static_cast<int>(eBPFPluginType::PROCESS));
    if (it != lib_funcs_.end()) {
      auto funcs = it->second;
      auto f = (update_func)funcs[process_probe_func::PROCESS_UPDATE];
      f(config);
      return true;
    } else {
      return false;
    }
    break;
  }
  default:
    break;
  }

  return false;
}

bool SourceManager::DynamicLibLoadSucess(eBPFPluginType type) {
    auto it = lib_funcs_.find(static_cast<int>(type));
    if (it == lib_funcs_.end()) {
        return false;
    }
    for (auto& x : it->second) {
        if (x == nullptr) return false;
    }
    return true;
}

bool SourceManager::LoadAndStartDynamicLib(eBPFPluginType type, void* config) {
    bool res;
    switch (type)
    {
    case eBPFPluginType::SOCKETTRACE:{
      if (!m_libs_[static_cast<int>(type)] || !DynamicLibLoadSucess(type)) {
        res = LoadSockettraceProbe();
        if (!res) return false;
        else return StartSockettraceProbe(static_cast<SockettraceConfig*>(config));
      }
      break;
    }
    case eBPFPluginType::PROCESS:{
      if (!m_libs_[static_cast<int>(type)] || !DynamicLibLoadSucess(type)) {
        res = LoadProcessProbe();
        if (!res) return false;
        else return StartProcessProbe(static_cast<SecureConfig*>(config));
      }
      break;
    }
    default:
      break;
    }
    
    return true;
}

bool SourceManager::StartSockettraceProbe(SockettraceConfig* config) {
  if (!DynamicLibLoadSucess(eBPFPluginType::SOCKETTRACE)) {
    // no init yet ... 
    LOG_ERROR(sLogger, ("libsockettrace load failed", ""));
    return false;
  }
  if (running_[static_cast<int>(eBPFPluginType::SOCKETTRACE)]) return true;
  auto it = lib_funcs_.find(static_cast<int>(eBPFPluginType::SOCKETTRACE));
  if (it == lib_funcs_.end()) {
    LOG_ERROR(sLogger, ("cannot find lib funcs for ", "sockettrace"));
    return false;
  }
  auto& t_funcs = it->second;

  // set offset configs
  // init config
  // this memory will be managed by plugin
  auto tmp_lib = m_libs_[static_cast<int>(eBPFPluginType::SOCKETTRACE)];
  auto binary_path_ = GetProcessExecutionDir();
  config->so_ = std::filesystem::path(binary_path_) / "libsockettrace.so";
  config->so_size_ = config->so_.length();

  config->uprobe_offset_ = LOAD_UPROBE_OFFSET(t_funcs[socket_trace_func::SOCKET_TRACE_CLEAN_UP_DOG]);
  config->upcr_offset_ = LOAD_UPROBE_OFFSET(t_funcs[socket_trace_func::SOCKET_TRACE_UPDATE_CONN_ROLE]);
  config->upps_offset_ = LOAD_UPROBE_OFFSET(t_funcs[socket_trace_func::SOCKET_TRACE_DISABLE_PROCESS]);
  config->upca_offset_ = LOAD_UPROBE_OFFSET(t_funcs[socket_trace_func::SOCKET_TRACE_UPDATE_CONN_ADDR]);

  auto& funcs = it->second;
  auto init = (init_func)funcs[socket_trace_func::SOCKET_TRACE_INIT];
  if (!init) return false;
  init((void*)config);
  return true;
}

bool SourceManager::StopSockettraceProbe() {
  // call deinit
  if (!DynamicLibLoadSucess(eBPFPluginType::SOCKETTRACE)) {
    // no init yet ... 
    return true;
  }
  if (!running_[static_cast<int>(eBPFPluginType::SOCKETTRACE)]) return true;
  auto it = lib_funcs_.find(static_cast<int>(eBPFPluginType::SOCKETTRACE));
  if (it == lib_funcs_.end()) {
    LOG_ERROR(sLogger, ("cannot find lib funcs for ", "sockettrace"));
    return false;
  }
  auto& funcs = it->second;
  auto deinit = (deinit_func)funcs[socket_trace_func::SOCKET_TRACE_DEINIT];
  if (!deinit) return false;
  deinit();
  return true;
}

bool SourceManager::StartProcessProbe(SecureConfig* config) {
  // call deinit
  if (!DynamicLibLoadSucess(eBPFPluginType::PROCESS)) {
    // no init yet ... 
    LOG_ERROR(sLogger, ("libsockettracesecure load failed", ""));
    return false;
  }
  if (running_[static_cast<int>(eBPFPluginType::PROCESS)]) return true;
  auto it = lib_funcs_.find(static_cast<int>(eBPFPluginType::PROCESS));
  if (it == lib_funcs_.end()) {
    LOG_ERROR(sLogger, ("cannot find lib funcs for ", "process"));
    return false;
  }
  auto& funcs = it->second;
  auto init = (init_func)funcs[process_probe_func::PROCESS_INIT];
  init((void*)config);
  return true;
}

bool SourceManager::StopProcessProbe() {
  // call deinit
  if (!DynamicLibLoadSucess(eBPFPluginType::PROCESS)) {
    // no init yet ... 
    return true;
  }
  if (!running_[static_cast<int>(eBPFPluginType::PROCESS)]) return true;
  auto it = lib_funcs_.find(static_cast<int>(eBPFPluginType::PROCESS));
  if (it == lib_funcs_.end()) {
    LOG_ERROR(sLogger, ("cannot find lib funcs for ", "process"));
    return false;
  }
  auto& funcs = it->second;
  auto deinit = (deinit_func)funcs[process_probe_func::PROCESS_DEINIT];
  if (!deinit) return false;
  deinit();
  return true;
}

bool SourceManager::LoadSockettraceProbe() {

  std::shared_ptr<DynamicLibLoader> tmp_lib = std::make_shared<DynamicLibLoader>();
  auto binary_path_ = GetProcessExecutionDir();
  LOG_INFO(sLogger, ("[SourceManager] load ebpf, libebpf path:", binary_path_));
  std::string loadErr;
  if (!tmp_lib->LoadDynLib("sockettrace", loadErr, binary_path_)) {
      LOG_ERROR(sLogger, ("[sockettrace] load ebpf dynamic library path", binary_path_)("error", loadErr));
      return false;
  }
  // init funcs
  std::vector<void*> t_funcs(socket_trace_func::SOCKET_TRACE_MAX, nullptr);
  // set offset configs
  // init config
  // this memory will be managed by plugin

  t_funcs[socket_trace_func::SOCKET_TRACE_INIT] = LOAD_EBPF_FUNC_ADDR(init);
  t_funcs[socket_trace_func::SOCKET_TRACE_UPDATE] = LOAD_EBPF_FUNC_ADDR(update);
  t_funcs[socket_trace_func::SOCKET_TRACE_DEINIT] = LOAD_EBPF_FUNC_ADDR(deinit);
  t_funcs[socket_trace_func::SOCKET_TRACE_CLEAN_UP_DOG] = LOAD_EBPF_FUNC_ADDR(ebpf_cleanup_dog);
  t_funcs[socket_trace_func::SOCKET_TRACE_UPDATE_CONN_ROLE] = LOAD_EBPF_FUNC_ADDR(ebpf_update_conn_role);
  t_funcs[socket_trace_func::SOCKET_TRACE_DISABLE_PROCESS] = LOAD_EBPF_FUNC_ADDR(ebpf_disable_process);
  t_funcs[socket_trace_func::SOCKET_TRACE_UPDATE_CONN_ADDR] = LOAD_EBPF_FUNC_ADDR(ebpf_update_conn_addr);

  // check function load success
  for (auto& x : t_funcs) {
    if (x == nullptr) return false;
  }

  // updaate meta
  lib_funcs_.insert({static_cast<int>(eBPFPluginType::SOCKETTRACE), std::move(t_funcs)});
  m_libs_[static_cast<int>(eBPFPluginType::SOCKETTRACE)] = std::move(tmp_lib);
  return true;
}

bool SourceManager::LoadProcessProbe() {
  std::shared_ptr<DynamicLibLoader> tmp_lib = std::make_shared<DynamicLibLoader>();
  auto binary_path_ = GetProcessExecutionDir();
  LOG_INFO(sLogger, ("[SourceManager] load ebpf, libebpf path:", binary_path_));
  std::string loadErr;
  if (!tmp_lib->LoadDynLib("sockettrace_secure", loadErr, binary_path_)) {
      LOG_ERROR(sLogger, ("[sockettrace_secure] load ebpf dynamic library path", binary_path_)("error", loadErr));
      return false;
  }

  // init funcs
  std::vector<void*> t_funcs(process_probe_func::PROCESS_MAX, nullptr);
  t_funcs[process_probe_func::PROCESS_INIT] = LOAD_EBPF_FUNC_ADDR(init);
  t_funcs[process_probe_func::PROCESS_UPDATE] = LOAD_EBPF_FUNC_ADDR(update);
  t_funcs[process_probe_func::PROCESS_DEINIT] = LOAD_EBPF_FUNC_ADDR(deinit);

  // updaate meta
  lib_funcs_.insert({static_cast<int>(eBPFPluginType::PROCESS), std::move(t_funcs)});
  m_libs_[static_cast<int>(eBPFPluginType::PROCESS)] = std::move(tmp_lib);

  return true;
}

bool SourceManager::UpdateSockettraceProbeConfig(SockettraceConfig* config) {
  // call deinit
  if (!DynamicLibLoadSucess(eBPFPluginType::SOCKETTRACE)) {
    // no init yet ... 
    return true;
  }
  if (!running_[static_cast<int>(eBPFPluginType::SOCKETTRACE)]) return false;
  auto it = lib_funcs_.find(static_cast<int>(eBPFPluginType::SOCKETTRACE));
  if (it == lib_funcs_.end()) {
    LOG_ERROR(sLogger, ("cannot find lib funcs for ", "sockettrace update"));
    return false;
  }
  auto& funcs = it->second;
  auto update = (update_func)funcs[socket_trace_func::SOCKET_TRACE_UPDATE];
  if (!update) return false;
  update((void*)config);
  return true;
}

bool SourceManager::UpdateProcessProbeConfig(SecureConfig* config) {
  // call update
  if (!DynamicLibLoadSucess(eBPFPluginType::PROCESS)) {
    // no init yet ... 
    return true;
  }
  if (!running_[static_cast<int>(eBPFPluginType::PROCESS)]) return false;
  auto it = lib_funcs_.find(static_cast<int>(eBPFPluginType::PROCESS));
  if (it == lib_funcs_.end()) {
    LOG_ERROR(sLogger, ("cannot find lib funcs for ", "process"));
    return false;
  }
  auto& funcs = it->second;
  auto update = (update_func)funcs[process_probe_func::PROCESS_UPDATE];
  if (!update) return false;
  update((void*)config);
  return true;
}

}
}