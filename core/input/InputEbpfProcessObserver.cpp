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

#include "input/InputEbpfProcessObserver.h"

#include "ebpf/observer/ObserverServer.h"

using namespace std;

namespace logtail {

const std::string InputEbpfProcessObserver::sName = "input_ebpf_processprobe_observer";

bool InputEbpfProcessObserver::Init(const Json::Value& config, uint32_t& pluginIdx, Json::Value& optionalGoPipeline) {
    // config string解析成定义的param
    return mObserverOptions.Init(ObserverType::PROCESS, config, mContext, sName);
}

bool InputEbpfProcessObserver::Start() {
    ObserverServer::GetInstance()->AddObserverOptions(mContext->GetConfigName(), mIndex, &mObserverOptions, mContext);
    ObserverServer::GetInstance()->Start();
    return true;
}

bool InputEbpfProcessObserver::Stop(bool isPipelineRemoving) {
    if (!isPipelineRemoving) {
        // TODO: ?
    }
    ObserverServer::GetInstance()->RemoveObserverOptions(mContext->GetConfigName(), mIndex);
    return true;
}

} // namespace logtail
