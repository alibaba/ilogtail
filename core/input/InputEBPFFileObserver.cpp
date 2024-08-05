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

#include "input/InputEBPFFileObserver.h"

#include "ebpf/eBPFServer.h"
#include "ebpf/config.h"

using namespace std;

namespace logtail {

const std::string InputEBPFFileObserver::sName = "input_ebpf_profilingprobe_observer";

// TODO: this plugin is not supported yet
bool InputEBPFFileObserver::Init(const Json::Value& config, uint32_t& pluginIdx, Json::Value& optionalGoPipeline) {
    return ebpf::InitObserverFileOption(config, mFileOption, mContext, sName);
}

// TODO: this plugin is not supported yet
bool InputEBPFFileObserver::Start() {
    return true;
}

// TODO: this plugin is not supported yet
bool InputEBPFFileObserver::Stop(bool isPipelineRemoving) {
    if (!isPipelineRemoving) {
        return true;
    }
    return true;
}

} // namespace logtail
