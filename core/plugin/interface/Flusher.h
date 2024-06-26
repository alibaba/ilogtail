/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <json/json.h>

#include "common/LogstoreSenderQueue.h"
#include "plugin/interface/Plugin.h"
#include "queue/SenderQueueItem.h"
#include "sdk/Common.h"

namespace logtail {

class Flusher : public Plugin {
public:
    virtual ~Flusher() = default;

    virtual bool Init(const Json::Value& config, Json::Value& optionalGoPipeline) = 0;
    virtual bool Register() { return true; }
    virtual bool Unregister(bool isPipelineRemoving) { return true; }
    virtual void Send(PipelineEventGroup&& g) = 0;
    virtual void Flush(size_t key) = 0;
    virtual void FlushAll() = 0;
    virtual sdk::AsynRequest* BuildRequest(SenderQueueItem* item) const = 0;
    // virtual void OnSucess() {}
    // virtual void OnFail() = 0;

    SingleLogstoreSenderManager<SenderQueueParam>* GetSenderQueue() const { return mSenderQueue; }

protected:
    // TODO: replace queue type
    SingleLogstoreSenderManager<SenderQueueParam>* mSenderQueue;
    // SenderQueue* mSenderQueue;
};

} // namespace logtail
