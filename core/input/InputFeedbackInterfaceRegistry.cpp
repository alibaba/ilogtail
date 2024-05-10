// Copyright 2024 iLogtail Authors
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

#include "input/InputFeedbackInterfaceRegistry.h"

#include "event/BlockEventManager.h"
#include "input/InputFile.h"
#include "input/InputContainerStdio.h"

namespace logtail {

void InputFeedbackInterfaceRegistry::LoadFeedbackInterfaces() {
    mInputFeedbackInterfaceMap[InputFile::sName] = BlockedEventManager::GetInstance();
    mInputFeedbackInterfaceMap[InputContainerStdio::sName] = BlockedEventManager::GetInstance();
}

FeedbackInterface* InputFeedbackInterfaceRegistry::GetFeedbackInterface(const std::string& name) const {
    auto iter = mInputFeedbackInterfaceMap.find(name);
    if (iter == mInputFeedbackInterfaceMap.end()) {
        return nullptr;
    }
    return iter->second;
}

} // namespace logtail
