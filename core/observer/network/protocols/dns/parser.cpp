// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <map>
#include <utility>
#include "parser.h"
#include "logger/Logger.h"
#include "interface/helper.h"
#include "StringTools.h"
#include "metas/ServiceMetaCache.h"
#include <string>

namespace logtail {

ParseResult DNSProtocolParser::OnPacket(PacketType pktType,
                                        MessageType msgType,
                                        PacketEventHeader* header,
                                        const char* pkt,
                                        int32_t pktSize,
                                        int32_t pktRealSize) {
    DNSParser dns(pkt, pktSize);
    try {
        dns.parse();
    } catch (const std::runtime_error& re) {
        LOG_DEBUG(sLogger,
                  ("dns_parse_fail", re.what())("data", charToHexString(pkt, pktSize, pktSize))(
                      "srcPort", header->SrcPort)("dstPort", header->DstPort));
        return ParseResult_Fail;
    } catch (const std::exception& ex) {
        LOG_DEBUG(sLogger,
                  ("dns_parse_fail", ex.what())("data", charToHexString(pkt, pktSize, pktSize))(
                      "srcPort", header->SrcPort)("dstPort", header->DstPort));
        return ParseResult_Fail;
    } catch (...) {
        LOG_DEBUG(
            sLogger,
            ("dns_parse_fail", "Unknown failure occurred when parse")("data", charToHexString(pkt, pktSize, pktSize))(
                "srcPort", header->SrcPort)("dstPort", header->DstPort));
        return ParseResult_Fail;
    }
    if (dns.OK()) {
        bool insertSuccess = true;
        uint16_t txid = dns.dnsHeader.txid;
        if (dns.packetType == DNSPacketRequest) {
            mCache.InsertReq(txid, [&](DNSRequestInfo* info) {
                info->TimeNano = header->TimeNano;
                info->ReqBytes = pktRealSize;
                if (dns.dnsRequest.requests.size() == 1) {
                    info->QueryRecord = dns.dnsRequest.requests.at(0).queryHost;
                    info->QueryType = ToString(dns.dnsRequest.requests.at(0).queryType);
                } else {
                    for (auto iter = dns.dnsRequest.requests.begin(); iter < dns.dnsRequest.requests.end(); iter++) {
                        info->QueryRecord.append(std::string(iter->queryHost)).append(",");
                        info->QueryType.append(std::string(ToString(iter->queryType))).append(",");
                    }
                    // remove last ,
                    if (!info->QueryRecord.empty()) {
                        info->QueryRecord.pop_back();
                    }
                    if (!info->QueryType.empty()) {
                        info->QueryType.pop_back();
                    }
                }
            });
        } else if (dns.packetType == DNSPacketResponse) {
            // currently, only collect the dns host outside the kubernetes cluster
            const static std::string sKubernetesDnsFlag = "cluster.local";
            const static auto sHostManager = ServiceMetaManager::GetInstance();
            for (auto& response : dns.dnsResponse.responses) {
                if (!EndWith(response.queryHost, sKubernetesDnsFlag)) {
                    sHostManager->AddHostName(header->PID, response.queryHost, response.value);
                }
            }
            mCache.InsertResp(txid, [&](DNSResponseInfo* info) -> bool {
                info->TimeNano = header->TimeNano;
                info->AnswerCount = dns.dnsResponse.answerCount;
                info->RespBytes = pktRealSize;
                return true;
            });
        }
        return insertSuccess ? ParseResult_OK : ParseResult_Drop;
    }
    return ParseResult_Fail;
}

bool DNSProtocolParser::GarbageCollection(size_t size_limit_bytes, uint64_t expireTimeNs) {
    return mCache.GarbageCollection(expireTimeNs);
}

int32_t DNSProtocolParser::GetCacheSize() {
    return mCache.GetRequestsSize() + mCache.GetResponsesSize();
}

} // end of namespace logtail