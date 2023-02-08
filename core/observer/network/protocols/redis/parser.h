/*
 * Copyright 2022 iLogtail Authors
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

#pragma once

#include <string>
#include <memory>
#include <deque>
#include <ostream>
#include "network/protocols/redis/type.h"
#include "observer/interface/network.h"
#include "inner_parser.h"
#include "network/protocols/buffer.h"
#include "network/protocols/parser.h"

namespace logtail {

struct RedisRequestInfo {
    int32_t ReqBytes{0};
    uint64_t TimeNano{0};
    std::string Data;

    std::string ToString() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }
    friend std::ostream& operator<<(std::ostream& os, const RedisRequestInfo& info);
};

struct RedisResponseInfo {
    uint64_t TimeNano{0};
    bool IsOK{false};
    std::string Data;
    int32_t RespBytes{0};
    std::string ToString() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }
    friend std::ostream& operator<<(std::ostream& os, const RedisResponseInfo& info);
};

typedef CommonCache<RedisRequestInfo, RedisResponseInfo, RedisProtocolEventAggregator, RedisProtocolEvent, 4>
    RedisCache;

// 协议解析器，流式解析，解析到某个协议后，自动放到aggregator中聚合
class RedisProtocolParser : public Parser {
public:
    RedisProtocolParser(RedisProtocolEventAggregator* aggregator,
                        CommonProtocolDetailsSampler* sampler,
                        PacketEventHeader* header)
        : mCache(aggregator), mKey(header), mSampler(sampler) {
        mCache.BindConvertFunc([&](RedisRequestInfo* req, RedisResponseInfo* resp, RedisProtocolEvent& event) {
            event.Info.LatencyNs = resp->TimeNano - req->TimeNano;
            if (event.Info.LatencyNs < 0) {
                event.Info.LatencyNs = 0;
            }
            event.Info.ReqBytes = req->ReqBytes;
            event.Info.RespBytes = resp->RespBytes;
            event.Key.ConnKey = mKey;
            event.Key.QueryCmd = ToLowerCaseString(req->Data.substr(0, req->Data.find_first_of(' ')));
            event.Key.Query = event.Key.QueryCmd;
            event.Key.Status = resp->IsOK;
            if (this->mSampler->IsSample(event.Key.Status, static_cast<int32_t>((event.Info.LatencyNs / 1e3)))) {
                ProtocolDetail detail;
                detail.Type = ProtocolType_Redis;
                detail.Request["cmd"] = req->Data;
                detail.Response["result"] = resp->Data;
                detail.Response["success"] = resp->IsOK ? "true" : "false";
                this->mSampler->AddData(std::move(detail));
            }
            return true;
        });
    }

    ~RedisProtocolParser() override = default;

    static RedisProtocolParser*
    Create(RedisProtocolEventAggregator* aggregator, CommonProtocolDetailsSampler* sampler, PacketEventHeader* header) {
        return new RedisProtocolParser(aggregator, sampler, header);
    }

    ParseResult OnPacket(PacketType pktType,
                         MessageType msgType,
                         const PacketEventHeader* header,
                         const char* pkt,
                         int32_t pktSize,
                         int32_t pktRealSize,
                         int32_t* offset) override;

    static void Delete(RedisProtocolParser* parser) { delete parser; }

    // GC，把内部没有完成Event匹配的消息按照SizeLimit和TimeOut进行清理
    // 返回值，如果是true代表还有数据，false代表无数据
    bool GarbageCollection(size_t size_limit_bytes, uint64_t expireTimeNs);

    int32_t GetCacheSize();

    size_t FindBoundary(const StringPiece& piece) override;

private:
    RedisCache mCache;
    CommonAggKey mKey;
    CommonProtocolDetailsSampler* mSampler;

    friend class ProtocolRedisUnittest;
};


} // namespace logtail
