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

#include <vector>
#include <string>
#include <memory>
#include <functional>


enum class SecureEventType {
  SECURE_EVENT_TYPE_SOCKET_SECURE,
  SECURE_EVENT_TYPE_FILE_SECURE,
  SECURE_EVENT_TYPE_PROCESS_SECURE,
  SECURE_EVENT_TYPE_MAX,
};

class AbstractSecurityEvent {
public:
  AbstractSecurityEvent(std::vector<std::pair<std::string, std::string>>&& tags,SecureEventType type, uint64_t ts)
    : tags_(tags), type_(type), timestamp_(ts) {}
  SecureEventType GetEventType() {return type_;}
  std::vector<std::pair<std::string, std::string>> GetAllTags() { return tags_; }
  uint64_t GetTimestamp() { return timestamp_; }
  void SetTimestamp(uint64_t ts) { timestamp_ = ts; }
  void AppendTags(std::pair<std::string, std::string>&& tag) {
    tags_.emplace_back(std::move(tag));
  }

private:
  std::vector<std::pair<std::string, std::string>> tags_;
  SecureEventType type_;
  uint64_t timestamp_;
};

class BatchAbstractSecurityEvent {
public:
  BatchAbstractSecurityEvent(){}
  std::vector<std::pair<std::string, std::string>> GetAllTags() { return tags_; }
  uint64_t GetTimestamp() { return timestamp_; }
private:
  std::vector<std::pair<std::string, std::string>> tags_;
  uint64_t timestamp_;
  std::vector<std::unique_ptr<AbstractSecurityEvent>> events;
};

//class ProcessSecurityEvent : public AbstractSecurityEvent {
//public:
//  ProcessSecurityEvent() {}
//private:
//
//};

using HandleSingleDataEventFn = std::function<void(std::unique_ptr<AbstractSecurityEvent>&& event)>;
using HandleBatchDataEventFn = std::function<void(std::vector<std::unique_ptr<AbstractSecurityEvent>>&& events)>;

//typedef void (*HandleDataEventFn)(void* ctx, std::unique_ptr<AbstractSecurityEvent> event);

enum UpdataType {
  SECURE_UPDATE_TYPE_ENABLE_PROBE = 0,
  SECURE_UPDATE_TYPE_CONFIG_CHAGE = 1,
  SECURE_UPDATE_TYPE_DISABLE_PROBE = 2,
  SECURE_UPDATE_TYPE_MAX = 3,
};

#define STRING_DEFAULT ""
// process
struct NamespaceFilter {
    // type of securityNamespaceFilter
    std::string mNamespaceType = STRING_DEFAULT;
    std::vector<std::string> mValueList;
};

// Config
struct SecureConfig {
public:
  SecureEventType plugin_type;
  bool enable_libbpf_debug_ = false;
  UpdataType type = UpdataType::SECURE_UPDATE_TYPE_ENABLE_PROBE;
  bool enable_probes_[static_cast<int>(SecureEventType::SECURE_EVENT_TYPE_MAX)];
  bool disable_probes_[static_cast<int>(SecureEventType::SECURE_EVENT_TYPE_MAX)];
  // common config
  std::string host_name_;
  std::string host_ip_;
  std::string host_path_prefix_;
  std::vector<std::string> network_call_names_;
  std::vector<std::string> process_call_names_;
  std::vector<std::string> file_call_names_;
  
  std::vector<NamespaceFilter> enable_namespaces_;
  std::vector<NamespaceFilter> disable_namespaces_;
  // process dynamic config
  std::vector<std::string> enable_pid_ns_;
  std::vector<std::string> disable_pid_ns_;

  HandleSingleDataEventFn proc_single_cb_;
  HandleBatchDataEventFn proc_batch_cb_;
  HandleSingleDataEventFn net_single_cb_;
  HandleBatchDataEventFn net_batch_cb_;

  // network dynamic config
  std::vector<std::string> enable_sips_;
  std::vector<std::string> disable_sips_;
  std::vector<std::string> enable_dips_;
  std::vector<std::string> disable_dips_;
  std::vector<uint32_t> enable_sports_;
  std::vector<uint32_t> enable_dports_;
  std::vector<uint32_t> disable_sports_;
  std::vector<uint32_t> disable_dports_;
};

