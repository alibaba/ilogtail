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

#include <mutex>

#include "pipeline/PipelineContext.h"
#include "queue/FeedbackQueueKey.h"

namespace logtail{
namespace ebpf {

class AbstractHandler {
public:
    AbstractHandler() {}
    AbstractHandler(logtail::QueueKey key, uint32_t idx) : mQueueKey(key), mPluginIdx(idx) {}
    void UpdateContext(logtail::QueueKey key, uint32_t index) { 
        mQueueKey = key;
        mPluginIdx = index;
    }
protected:
    logtail::QueueKey mQueueKey = 0;
    uint64_t mProcessTotalCnt = 0;
    uint32_t mPluginIdx = 0;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class eBPFServerUnittest;
#endif
};

}
}
