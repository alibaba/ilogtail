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

#include "input/InputEbpfFileSecurity.h"


using namespace std;

namespace logtail {

const std::string InputEbpfFileSecurity::sName = "input_ebpf_fileprobe_security";

bool InputEbpfFileSecurity::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    mDetail = config.toStyledString();
    // todo config string解析成定义的param
    return true;
}

bool InputEbpfFileSecurity::Start() {
    // todo 1、security ebpf采集总线程未启动，调用SecurityServer的start启动
    // todo 2、插件相关配置注册到SecurityServer
    return true;
}

bool InputEbpfFileSecurity::Stop(bool isPipelineRemoving) {
    // todo 1、该ebpf类型的 security采集线程未暂停，调用SecurityServer的stop暂停
    // todo 2、插件相关配置从SecurityServer移除
    return true;
}

} // namespace logtail
