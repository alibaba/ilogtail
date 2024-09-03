/*
 * Copyright 2024 iLogtail Authors
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

#include "common/http/HttpResponse.h"
#include "pipeline/plugin/interface/Flusher.h"
#include "queue/SenderQueueItem.h"
#include "sink/http/HttpSinkRequest.h"

namespace logtail {

class HttpFlusher : public Flusher {
public:
    virtual ~HttpFlusher() = default;

    virtual std::unique_ptr<HttpSinkRequest> BuildRequest(SenderQueueItem* item) const = 0;
    virtual void OnSendDone(const HttpResponse& response, SenderQueueItem* item) = 0;

    virtual SinkType GetSinkType() override { return SinkType::HTTP; }
};

} // namespace logtail
