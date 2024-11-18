/*
 * Copyright 2024 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "CollectorManager.h"

#include "BaseCollector.h"
#include "MockCollector.h"
#include "host_monitor/collector/ProcessCollector.h"

namespace logtail {

CollectorManager::CollectorManager() {
    RegisterCollector<MockCollector>();
    RegisterCollector<ProcessCollector>();
}

std::shared_ptr<BaseCollector> CollectorManager::GetCollector(const std::string& collectorName) {
    auto it = mCollectorMap.find(collectorName);
    if (it == mCollectorMap.end()) {
        return nullptr;
    }
    return it->second;
}

template <typename T>
void CollectorManager::RegisterCollector() {
    auto collector = std::make_shared<T>();
    mCollectorMap[collector->GetName()] = collector;
}

} // namespace logtail
