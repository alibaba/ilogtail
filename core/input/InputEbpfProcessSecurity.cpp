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

#include "input/InputEbpfProcessSecurity.h"

#include "ebpf/security/SecurityServer.h"

using namespace std;

namespace logtail {

const std::string InputEbpfProcessSecurity::sName = "input_ebpf_processprobe_security";

bool InputEbpfProcessSecurity::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    // config string解析成定义的param
    return mSecurityOptions.Init(SecurityFilterType::PROCESS, config, mContext, sName);
}

bool InputEbpfProcessSecurity::Start() {
    SecurityServer::GetInstance()->AddSecurityOptions(mContext->GetConfigName(), &mSecurityOptions);
    SecurityServer::GetInstance()->Start();
    return true;
}

bool InputEbpfProcessSecurity::Stop(bool isPipelineRemoving) {
    if (!isPipelineRemoving) {
        // TODO: ?
    }
    SecurityServer::GetInstance()->RemoveSecurityOptions(mContext->GetConfigName());
    return true;
}

} // namespace logtail
