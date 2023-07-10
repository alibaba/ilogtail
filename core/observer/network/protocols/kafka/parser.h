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
#include "type.h"
#include "observer/interface/network.h"
#include "inner_parser.h"

namespace logtail {

struct KafkaRequestInfo {
    KafkaApiType ApiKey;
    uint16_t Version;
    int32_t ReqBytes;
    uint64_t TimeNano;
    std::string Topic;

    void Clear() {
        ApiKey = KafkaApiType::Produce;
        TimeNano = 0;
        Version = 0;
        ReqBytes = 0;
        TimeNano = 0;
        Topic.clear();
    }
    friend std::ostream& operator<<(std::ostream& os, const KafkaRequestInfo& info) {
        os << "ApiKey: " << static_cast<uint8_t>(info.ApiKey) << " Version: " << info.Version
           << " ReqBytes: " << info.ReqBytes << " TimeNano: " << info.TimeNano << " Topic: " << info.Topic;
        return os;
    }
    std::string ToString() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }
};

struct KafkaResponseInfo {
    uint64_t TimeNano;
    int32_t RespBytes;
    uint16_t Code;
    std::string Topic;

    void Clear() {
        TimeNano = 0;
        Code = 0;
        TimeNano = 0;
        Topic.clear();
    }
    friend std::ostream& operator<<(std::ostream& os, const KafkaResponseInfo& info) {
        os << "TimeNano: " << info.TimeNano << " RespBytes: " << info.RespBytes << " Code: " << info.Code;
        return os;
    }
    std::string ToString() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }
};

using KafkaCache = CommonMapCache<KafkaRequestInfo,
                                  KafkaResponseInfo,
                                  uint64_t,
                                  KafkaProtocolEventAggregator,
                                  KafkaProtocolEvent,
                                  4>;


class KafkaProtocolParser {
public:
    KafkaProtocolParser(KafkaProtocolEventAggregator* aggregator, PacketEventHeader* header)
        : mCache(aggregator), mKey(header) {
        mCache.BindConvertFunc(
            [&](KafkaRequestInfo* requestInfo, KafkaResponseInfo* responseInfo, KafkaProtocolEvent& event) {
                event.Info.LatencyNs = int64_t(responseInfo->TimeNano - requestInfo->TimeNano);
                if (event.Info.LatencyNs < 0) {
                    event.Info.LatencyNs = 0;
                }
                event.Info.ReqBytes = requestInfo->ReqBytes;
                event.Info.RespBytes = responseInfo->RespBytes;
                event.Key.Version = std::to_string(requestInfo->Version);
                event.Key.ReqDomain
                    = requestInfo->Topic.empty() ? std::move(responseInfo->Topic) : std::move(requestInfo->Topic);
                event.Key.RespCode = static_cast<int16_t>(responseInfo->Code);
                event.Key.ReqType = KafkaApiTypeToString(requestInfo->ApiKey);
                event.Key.ConnKey = mKey;
                return true;
            });
    }

    static KafkaProtocolParser* Create(KafkaProtocolEventAggregator* aggregator, PacketEventHeader* header) {
        return new KafkaProtocolParser(aggregator, header);
    }

    static void Delete(KafkaProtocolParser* parser) { delete parser; }

    ParseResult OnPacket(PacketType pktType,
                         MessageType msgType,
                         PacketEventHeader* header,
                         const char* pkt,
                         int32_t pktSize,
                         int32_t pktRealSize);

    bool GarbageCollection(size_t size_limit_bytes, uint64_t expireTimeNs);

    int32_t GetCacheSize();

    bool InsertRespCache(uint16_t id, const char* data, uint16_t len);

private:
    KafkaCache mCache;
    CommonAggKey mKey;
    std::map<uint16_t, std::string> mRespCache;
    friend class ProtocolKafkaUnittest;
};
} // namespace logtail
