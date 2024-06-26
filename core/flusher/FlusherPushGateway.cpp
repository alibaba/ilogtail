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

#include "FlusherPushGateway.h"

#include <vector>

#include "sdk/Common.h"
#include "sender/Sender.h"

using namespace std;
using namespace logtail::sdk;

namespace logtail {

const string FlusherPushGateway::sName = "flusher_push_gateway";
bool FlusherPushGateway::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    if (config.isMember("pushGatewayScheme")) {
        const Json::Value& pushGatewayScheme = config["pushGatewayScheme"];
        mPushGatewayScheme = pushGatewayScheme.asString();
    } else {
        // TODO: throws exception
    }
    if (config.isMember("pushGatewayHost")) {
        const Json::Value& pushGatewayHost = config["pushGatewayHost"];
        mPushGatewayHost = pushGatewayHost.asString();
    } else {
        // TODO: throws exception
    }
    if (config.isMember("pushGatewayPath")) {
        const Json::Value& pushGatewayPath = config["pushGatewayPath"];
        mPushGatewayPath = pushGatewayPath.asString();
    } else {
        // TODO: throws exception
    }
    DefaultFlushStrategyOptions strategy{
        static_cast<uint32_t>(1024 * 1024), static_cast<uint32_t>(5000), static_cast<uint32_t>(1)};
    if (!mBatcher.Init(Json::Value(), this, strategy, false)) {
        // TODO: throws exception
    }
    mItemSender.reset(new SystemSender);
    mSerializer.reset(new PushGatewaySerializer);
    return true;
}

void FlusherPushGateway::SerializeAndPush(BatchedEventsList&& groupList) {
    for (auto& batchedEv : groupList) {
        string data, errMsg;
        mSerializer->Serialize(std::move(batchedEv), data, errMsg);
        // TODO: mQueueKey && groupStrategy
        SenderQueueItem* item
            = new SenderQueueItem(std::move(data), data.size(), this, 0, RawDataType::EVENT_GROUP, false);
        mItemSender->Send(item);
    }
}

void FlusherPushGateway::SerializeAndPush(vector<BatchedEventsList>&& groupLists) {
    for (auto& groupList : groupLists) {
        SerializeAndPush(std::move(groupList));
    }
}

void FlusherPushGateway::Send(PipelineEventGroup&& g) {
    vector<BatchedEventsList> res;
    mBatcher.Add(std::move(g), res);
    SerializeAndPush(std::move(res));
}

void FlusherPushGateway::Flush(size_t key) {
    BatchedEventsList res;
    mBatcher.FlushQueue(key, res);
    SerializeAndPush(std::move(res));
}
void FlusherPushGateway::FlushAll() {
    vector<BatchedEventsList> res;
    mBatcher.FlushAll(res);
    SerializeAndPush(std::move(res));
}

AsynRequest* FlusherPushGateway::BuildRequest(SenderQueueItem* item) const {
    PromiseClosure* promiseClosure = new PromiseClosure;
    Response* response = new Response();
    string httpMethod = "POST";
    bool httpsFlag = mPushGatewayScheme == "https";
    int32_t port = httpsFlag ? 443 : 80;
    auto headers = new std::map<std::string, std::string>;
    headers->insert(std::make_pair("Content-Type", "text/plain; version=0.0.4; charset=utf-8"));
    return new AsynRequest("POST",
                           mPushGatewayHost,
                           port,
                           mPushGatewayPath,
                           "",
                           *headers,
                           item->mData,
                           30,
                           "",
                           httpsFlag,
                           promiseClosure,
                           response);
}

void SystemSender::Send(SenderQueueItem* item) {
    Sender::Instance()->PutIntoBatchMap(item);
}

void PromiseClosure::OnSuccess(Response* response) {
    mPromise.set_value(ResponseInfo{
        response : response,
        errorCode : "",
        errorMessage : "",
    });
}

void PromiseClosure::OnFail(Response* response, const std::string& errorCode, const std::string& errorMessage) {
    mPromise.set_value(ResponseInfo{
        response : response,
        errorCode : errorCode,
        errorMessage : errorMessage,
    });
}

#ifdef APSARA_UNIT_TEST_MAIN
void TestSender::Send(SenderQueueItem* item) {
    mItems.push_back(item);
}

#endif

} // namespace logtail
