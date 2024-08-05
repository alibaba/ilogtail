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

#include "input/InputEBPFNetworkSecurity.h"

#include "ebpf/include/export.h"
#include "ebpf/eBPFServer.h"


using namespace std;

namespace logtail {

const std::string InputEBPFNetworkSecurity::sName = "input_ebpf_sockettraceprobe_security";

bool InputEBPFNetworkSecurity::Init(const Json::Value& config, uint32_t& pluginIdx, Json::Value& optionalGoPipeline) {
    return mSecurityOptions.Init(ebpf::SecurityFilterType::NETWORK, config, mContext, sName);
}

bool InputEBPFNetworkSecurity::Start() {
    return ebpf::eBPFServer::GetInstance()->EnablePlugin(mContext->GetConfigName(), mIndex, nami::PluginType::NETWORK_SECURITY, mContext, &mSecurityOptions);
}

bool InputEBPFNetworkSecurity::Stop(bool isPipelineRemoving) {
    if (!isPipelineRemoving) {
        ebpf::eBPFServer::GetInstance()->SuspendPlugin(mContext->GetConfigName(), nami::PluginType::NETWORK_SECURITY);
        return true;
    }
    return ebpf::eBPFServer::GetInstance()->DisablePlugin(mContext->GetConfigName(), nami::PluginType::NETWORK_SECURITY);
}

} // namespace logtail
