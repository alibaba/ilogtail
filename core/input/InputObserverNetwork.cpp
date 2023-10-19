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

#include "input/InputObserverNetwork.h"
#include "InputObserverNetwork.h"

using namespace std;

namespace logtail {

const std::string InputObserverNetwork::sName = "input_observer_network";

bool logtail::InputObserverNetwork::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    mDetail = config.toStyledString();
    optionalGoPipeline["mix_process_mode"] = "observer";
    return true;
}

bool logtail::InputObserverNetwork::Start() {
    return true;
}

bool logtail::InputObserverNetwork::Stop(bool isPipelineRemoving) {
    return true;
}

} // namespace logtail
