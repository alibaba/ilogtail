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
#include "type.h"
#include "observer/interface/network.h"

namespace logtail {

struct DubboRequestInfo {
    int32_t ReqBytes;
    uint64_t TimeNano;
    std::string Version;
    std::string ServiceVersion;
    std::string Method;
    std::string Service;

    friend std::ostream& operator<<(std::ostream& os, const DubboRequestInfo& info) {
        os << "TimeNano: " << info.TimeNano << " Version: " << info.Version
           << " ServiceVersion: " << info.ServiceVersion << " Method: " << info.Method << " Service: " << info.Service
           << " ReqBytes: " << info.ReqBytes;
        return os;
    }

    std::string ToString() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }

    void Clear() {
        TimeNano = 0;
        Version.clear();
        ServiceVersion.clear();
        Method.clear();
        Service.clear();
        ReqBytes = 0;
    }
};
struct DubboResponseInfo {
    int16_t RespCode;
    int32_t RespBytes;
    uint64_t TimeNano;

    friend std::ostream& operator<<(std::ostream& os, const DubboResponseInfo& info) {
        os << "TimeNano: " << info.TimeNano << " RespCode: " << info.RespCode << " RespBytes: " << info.RespBytes;
        return os;
    }
    std::string ToString() const {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }
    void Clear() {
        RespCode = 0;
        RespBytes = 0;
        TimeNano = 0;
    }
};


using DubboCache = CommonMapCache<DubboRequestInfo,
                                  DubboResponseInfo,
                                  uint64_t,
                                  DubboProtocolEventAggregator,
                                  DubboProtocolEvent,
                                  4>;
class DubboProtocolParser {
public:
    explicit DubboProtocolParser(DubboProtocolEventAggregator* aggregator, PacketEventHeader* header)
        : mCache(aggregator), mKey(header) {
        mCache.BindConvertFunc(
            [&](DubboRequestInfo* requestInfo, DubboResponseInfo* responseInfo, DubboProtocolEvent& event) -> bool {
                event.Info.LatencyNs = int64_t(responseInfo->TimeNano - requestInfo->TimeNano);
                if (event.Info.LatencyNs < 0) {
                    event.Info.LatencyNs = 0;
                }
                event.Info.ReqBytes = requestInfo->ReqBytes;
                event.Info.RespBytes = responseInfo->RespBytes;
                event.Key.Version = std::move(requestInfo->Version);
                event.Key.ReqDomain = std::move(requestInfo->Service);
                event.Key.ReqResource = std::move(requestInfo->Method);
                event.Key.RespCode = responseInfo->RespCode;
                event.Key.ReqType = "rpc";
                event.Key.ConnKey = mKey;
                return true;
            });
    }

    static DubboProtocolParser* Create(DubboProtocolEventAggregator* aggregator, PacketEventHeader* header) {
        return new DubboProtocolParser(aggregator, header);
    }

    static void Delete(DubboProtocolParser* parser) { delete parser; }

    ParseResult OnPacket(PacketType pktType,
                         MessageType msgType,
                         PacketEventHeader* header,
                         const char* pkt,
                         int32_t pktSize,
                         int32_t pktRealSize);

    bool GarbageCollection(size_t size_limit_bytes, uint64_t expireTimeNs);

    int32_t GetCacheSize();


private:
    DubboCache mCache;
    CommonAggKey mKey;
};
} // end of namespace logtail
