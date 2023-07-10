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

#include <memory>
#include <iostream>
#include "parser.h"
#include "inner_parser.h"
#include "logger/Logger.h"
#include "interface/helper.h"

namespace logtail {


ParseResult KafkaProtocolParser::OnPacket(PacketType pktType,
                                          MessageType msgType,
                                          PacketEventHeader* header,
                                          const char* pkt,
                                          int32_t pktSize,
                                          int32_t pktRealSize) {
    KafkaParser kafka(pkt, pktSize);
    LOG_TRACE(sLogger,
              ("message_type", MessageTypeToString(msgType))("Kafka date", charToHexString(pkt, pktSize, pktSize)));
    try {
        switch (msgType) {
            case MessageType_Request: {
                kafka.ParseRequest();
            } break;
            case MessageType::MessageType_Response: {
                kafka.ParseResponseHeader();
                if (!kafka.OK())
                    return ParseResult_Fail;
                KafkaRequestInfo* req = this->mCache.FindReq(kafka.mData.CorrelationId);
                if (req == nullptr) {
                    this->InsertRespCache(kafka.mData.CorrelationId, kafka.head(), kafka.getLeftSize());
                    return ParseResult_OK;
                } else {
                    kafka.ParseResponse(req->ApiKey, req->Version);
                }
                break;
            }
            default:
                return ParseResult_Drop;
        }
    } catch (const std::runtime_error& re) {
        LOG_DEBUG(sLogger,
                  ("Kafka_parse_fail", re.what())("data", charToHexString(pkt, pktSize, pktSize))(
                      "srcPort", header->SrcPort)("dstPort", header->DstPort));
        return ParseResult_Fail;
    } catch (const std::exception& ex) {
        LOG_DEBUG(sLogger,
                  ("Kafka_parse_fail", ex.what())("data", charToHexString(pkt, pktSize, pktSize))(
                      "srcPort", header->SrcPort)("dstPort", header->DstPort));
        return ParseResult_Fail;
    } catch (...) {
        LOG_DEBUG(
            sLogger,
            ("Kafka_parse_fail", "Unknown failure occurred when parse")("data", charToHexString(pkt, pktSize, pktSize))(
                "srcPort", header->SrcPort)("dstPort", header->DstPort));
        return ParseResult_Fail;
    }
    if (!kafka.OK() || static_cast<uint8_t>(kafka.mData.Type) > 2) {
        LOG_DEBUG(sLogger,
                  ("Kafka_parse_fail", KafkaMsgTypeToString(kafka.mData.Type))(
                      "data", charToHexString(pkt, pktSize, pktSize))("srcPort", header->SrcPort)("dstPort",
                                                                                                  header->DstPort));
        return ParseResult_Fail;
    }

    bool insertSuccess = true;
    if (msgType == MessageType_Request) {
        insertSuccess = mCache.InsertReq(kafka.mData.CorrelationId, [&](KafkaRequestInfo* info) {
            info->TimeNano = header->TimeNano;
            info->ReqBytes = pktRealSize;
            info->Topic = std::string(kafka.mData.Request.Topic.data(), kafka.mData.Request.Topic.size());
            info->ApiKey = kafka.mData.Request.ApiKey;
            info->Version = kafka.mData.Request.Version;
            LOG_TRACE(sLogger, ("Kafka insert req", info->ToString()));
        });
    } else {
        insertSuccess = mCache.InsertResp(kafka.mData.CorrelationId, [&](KafkaResponseInfo* info) {
            info->TimeNano = header->TimeNano;
            info->RespBytes = pktRealSize;
            info->Code = kafka.mData.Response.Code;
            info->Topic = std::string(kafka.mData.Response.Topic.data(), kafka.mData.Response.Topic.size());
            LOG_TRACE(sLogger, ("Kafka insert resp", info->ToString()));
        });
    }
    return insertSuccess ? ParseResult_OK : ParseResult_Drop;
}

bool KafkaProtocolParser::GarbageCollection(size_t size_limit_bytes, uint64_t expireTimeNs) {
    return mCache.GarbageCollection(expireTimeNs);
}

int32_t KafkaProtocolParser::GetCacheSize() {
    return this->mCache.GetRequestsSize() + this->mCache.GetResponsesSize();
}

bool KafkaProtocolParser::InsertRespCache(uint16_t id, const char* data, uint16_t len) {
    // currently, only cache 10 resp data for wait pair
    if (this->mRespCache.size() > 10) {
        this->mRespCache.erase(this->mRespCache.begin());
    }
    this->mRespCache.insert(std::make_pair(id, std::string(data, len)));
    return true;
}


} // end of namespace logtail