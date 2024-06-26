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

#include <string>
#include <vector>

#include "batch/Batcher.h"
#include "plugin/interface/Flusher.h"
#include "sdk/Closure.h"
#include "serializer/PushGatewaySerializer.h"

namespace logtail {

class ItemSender {
public:
    virtual void Send(SenderQueueItem* item) = 0;
};

class SystemSender : public ItemSender {
public:
    void Send(SenderQueueItem* item) override;
};

#ifdef APSARA_UNIT_TEST_MAIN

class TestSender : public ItemSender {
public:
    std::vector<SenderQueueItem*> mItems;
    void Send(SenderQueueItem* item) override;
};

#endif

struct ResponseInfo {
    sdk::Response* response;
    std::string errorCode;
    std::string errorMessage;
};

class PromiseClosure : public sdk::LogsClosure {
public:
    void Done() override {};
    void OnSuccess(sdk::Response* response) override;
    void OnFail(sdk::Response* response, const std::string& errorCode, const std::string& errorMessage) override;
    std::promise<ResponseInfo> mPromise;
};

class FlusherPushGateway : public Flusher {
public:
    static const std::string sName;

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config, Json::Value& optionalGoPipeline) override;
    void SerializeAndPush(BatchedEventsList&& groupLists);
    void SerializeAndPush(std::vector<BatchedEventsList>&& groupLists);
    void Send(PipelineEventGroup&& g) override;
    void Flush(size_t key) override;
    void FlushAll() override;
    sdk::AsynRequest* BuildRequest(SenderQueueItem* item) const override;
    // PushGatewaySendClosure* GetLastSendClosure();

private:
    Batcher<> mBatcher;
    std::unique_ptr<ItemSender> mItemSender;
    // PushGatewaySendClosure* lastSendClosure;
    std::unique_ptr<PushGatewaySerializer> mSerializer;
    std::string mPushGatewayScheme;
    std::string mPushGatewayHost;
    std::string mPushGatewayPath;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class FlusherPushGatewayUnittest;
#endif
};

} // namespace logtail
