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

#include <map>
#include <ostream>
#include "network/protocols/http/type.h"
#include "observer/interface/network.h"
#include "inner_parser.h"
#include "network/protocols/parser.h"

namespace logtail {

struct HTTPRequestInfo {
    uint64_t TimeNano;
    std::string Method;
    std::string URL;
    std::string Version;
    std::string Host;
    int32_t ReqBytes;
    Json::Value Headers;
    Json::Value Body;

    friend std::ostream& operator<<(std::ostream& os, const HTTPRequestInfo& info) {
        os << "TimeNano: " << info.TimeNano << " Method: " << info.Method << " URL: " << info.URL
           << " Version: " << info.Version << " Host: " << info.Host << " ReqBytes: " << info.ReqBytes;
        return os;
    }

    std::string ToString() {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }
};
struct HTTPResponseInfo {
    uint64_t TimeNano;
    int16_t RespCode;
    int32_t RespBytes;
    Json::Value Headers;
    Json::Value Body;

    friend std::ostream& operator<<(std::ostream& os, const HTTPResponseInfo& info) {
        os << "TimeNano: " << info.TimeNano << " RespCode: " << info.RespCode << " RespBytes: " << info.RespBytes;
        return os;
    }
    std::string ToString() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }
};

typedef CommonCache<HTTPRequestInfo, HTTPResponseInfo, HTTPProtocolEventAggregator, HTTPProtocolEvent, 4> HttpCache;


// 协议解析器，流式解析，解析到某个协议后，自动放到aggregator中聚合
class HTTPProtocolParser : public Parser {
private:
    HttpCache mCache;
    CommonAggKey mKey;
    CommonProtocolDetailsSampler* mSampler;

public:
    static HTTPProtocolParser*
    Create(HTTPProtocolEventAggregator* aggregator, CommonProtocolDetailsSampler* sampler, PacketEventHeader* header) {
        return new HTTPProtocolParser(aggregator, sampler, header);
    }
    static void Delete(HTTPProtocolParser* parser) { delete parser; }


    explicit HTTPProtocolParser(HTTPProtocolEventAggregator* aggregator,
                                CommonProtocolDetailsSampler* sampler,
                                PacketEventHeader* header)
        : mCache(aggregator), mKey(header), mSampler(sampler) {
        mCache.BindConvertFunc(
            [&](HTTPRequestInfo* requestInfo, HTTPResponseInfo* responseInfo, HTTPProtocolEvent& event) -> bool {
                event.Info.LatencyNs = responseInfo->TimeNano - requestInfo->TimeNano;
                if (event.Info.LatencyNs < 0) {
                    event.Info.LatencyNs = 0;
                }
                event.Info.ReqBytes = requestInfo->ReqBytes;
                event.Info.RespBytes = responseInfo->RespBytes;
                event.Key.ReqType = std::move(requestInfo->Method);
                event.Key.ReqDomain = std::move(requestInfo->Host);
                event.Key.ReqResource = std::move(requestInfo->URL);
                event.Key.Version = std::move(requestInfo->Version);
                event.Key.RespCode = responseInfo->RespCode;
                event.Key.ConnKey = mKey;
                if (this->mSampler->IsSample(event.Key.RespCode < 400,
                                             static_cast<int32_t>((event.Info.LatencyNs / 1e3)))) {
                    ProtocolDetail detail;
                    detail.Type = ProtocolType_HTTP;
                    detail.Request["version"] = event.Key.Version;
                    detail.Response["code"] = event.Key.RespCode;
                    detail.Request["headers"] = std::move(requestInfo->Headers);
                    detail.Request["body"] = std::move(requestInfo->Body);
                    detail.Response["headers"] = std::move(responseInfo->Headers);
                    detail.Response["body"] = std::move(responseInfo->Body);
                    detail.ReqDomain = event.Key.ReqDomain;
                    detail.ReqResource = event.Key.ReqResource;
                    detail.ReqType = event.Key.ReqType;
                    this->mKey.ToPB(&detail.Tags);
                    this->mSampler->AddData(std::move(detail));
                }
                return true;
            });
    }

    ~HTTPProtocolParser() override = default;

    ParseResult OnPacket(PacketType pktType,
                         MessageType msgType,
                         const PacketEventHeader* header,
                         const char* pkt,
                         int32_t pktSize,
                         int32_t pktRealSize,
                         int32_t* offset) override;
    size_t FindBoundary(MessageType message_type, const StringPiece& piece) override;
    bool GarbageCollection(size_t size_limit_bytes, uint64_t expireTimeNs);
    size_t GetCacheSize() override;


    friend class ProtocolHttpUnittest;
};


} // end of namespace logtail
