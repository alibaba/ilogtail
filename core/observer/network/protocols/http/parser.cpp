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
#include <string.h>
#include "parser.h"
#include "inner_parser.h"
#include "observer/interface/helper.h"
#include "logger/Logger.h"

namespace logtail {


ParseResult HTTPProtocolParser::OnPacket(PacketType pktType,
                                         MessageType msgType,
                                         const PacketEventHeader* header,
                                         const char* pkt,
                                         int32_t pktSize,
                                         int32_t pktRealSize,
                                         int32_t* offset) {
    HTTPParser parser(pkt + *offset, static_cast<size_t>((pktSize - *offset)));
    ParseResult res = ParseResult_Drop;
    if (msgType == MessageType_Request) {
        res = parser.ParseRequest();
    } else if (msgType == MessageType_Response) {
        res = parser.ParseResp();
    }
    LOG_TRACE(sLogger,
              ("http got data", std::string(pkt, pktSize))("message_type", MessageTypeToString(msgType))(
                  "raw_data", charToHexString(pkt, pktSize, pktSize))("connection_id", header->SockHash));
    if (res != ParseResult_OK) {
        return res;
    }
    *offset = *offset + parser.GetCostOffset();
    bool insertSuccess = true;
    if (msgType == MessageType_Request) {
        auto iter = parser.GetPacket().Common.Headers.find("Host");
        std::string host;
        if (iter != parser.GetPacket().Common.Headers.end()) {
            host = std::string(iter->second.data(), iter->second.size());
        } else {
            if (host.empty()) {
                if (pktType == PacketType_Out) {
                    host = SockAddressToString(header->DstAddr);
                } else {
                    host = SockAddressToString(header->SrcAddr);
                }
            }
        }
        size_t pos = StringPiece(parser.GetPacket().Req.Url, parser.GetPacket().Req.UrlLen).find("?");
        std::string url
            = std::string(parser.GetPacket().Req.Url, pos == StringPiece::npos ? parser.GetPacket().Req.UrlLen : pos);
        insertSuccess = mCache.InsertReq([&](HTTPRequestInfo* req) {
            req->TimeNano = header->TimeNano;
            req->Method = {parser.GetPacket().Req.Method, parser.GetPacket().Req.MethodLen};
            req->URL = std::move(url);
            req->Version = std::to_string(parser.GetPacket().Common.Version);
            req->Host = std::move(host);
            req->ReqBytes = pktRealSize;
            req->Body = Json::Value(parser.GetPacket().Req.Body.data(),
                                    parser.GetPacket().Req.Body.data() + parser.GetPacket().Req.Body.size());
            for (const auto& Header : parser.GetPacket().Common.Headers) {
                req->Headers[std::string(Header.first.data(), Header.first.size())]
                    = std::string(Header.second.data(), Header.second.size());
            }
            LOG_TRACE(sLogger, ("http insert req hash", header->SockHash)("data", req->ToString()));
        });
    } else if (msgType == MessageType_Response) {
        insertSuccess = mCache.InsertResp([&](HTTPResponseInfo* info) {
            info->TimeNano = header->TimeNano;
            info->RespCode = parser.GetPacket().Resp.Code;
            info->RespBytes = pktRealSize;
            info->Body = Json::Value(parser.GetPacket().Resp.Body.data(),
                                     parser.GetPacket().Resp.Body.data() + parser.GetPacket().Resp.Body.size());
            for (const auto& Header : parser.GetPacket().Common.Headers) {
                info->Headers[std::string(Header.first.data(), Header.first.size())]
                    = std::string(Header.second.data(), Header.second.size());
            }
            LOG_TRACE(sLogger, ("http insert resp hash", header->SockHash)("data", info->ToString()));
        });
    }
    return insertSuccess ? ParseResult_OK : ParseResult_Drop;
}

bool HTTPProtocolParser::GarbageCollection(size_t size_limit_bytes, uint64_t expireTimeNs) {
    bool pok = Parser::GarbageCollection(size_limit_bytes, expireTimeNs);
    bool cok = mCache.GarbageCollection(expireTimeNs);
    return pok && cok;
}

size_t HTTPProtocolParser::GetCacheSize() {
    return mCache.GetRequestsSize() + mCache.GetResponsesSize();
}
size_t HTTPProtocolParser::FindBoundary(MessageType message_type, const StringPiece& piece) {
    static const std::array<StringPiece, 2> kRespFlags = {"HTTP/1.1 ", "HTTP/1.0 "};
    static const std::array<StringPiece, 9> kReqFlags
        = {"GET ", "HEAD ", "POST ", "PUT ", "DELETE ", "CONNECT ", "OPTIONS ", "TRACE ", "PATCH "};
    static const StringPiece kEndFlag = "\r\n\r\n";
    const StringPiece* head;
    size_t len;
    switch (message_type) {
        case MessageType_Request:
            head = kReqFlags.data();
            len = kReqFlags.size();
            break;
        case MessageType_Response:
            head = kRespFlags.data();
            len = kRespFlags.size();
            break;
        default:
            return std::string::npos;
    }
    size_t start = 0;
    while (true) {
        size_t endPos = piece.find(kEndFlag, start);
        if (endPos == StringPiece::npos) {
            return std::string::npos;
        }
        StringPiece sub = piece.substr(start, endPos);
        size_t subPos = StringPiece::npos;
        const StringPiece* cur = head;
        for (size_t i = 0; i < len; ++i) {
            cur += i;
            auto pos = sub.rfind(*cur);
            if (pos != StringPiece::npos) {
                subPos = (subPos == StringPiece::npos) ? pos : std::max(subPos, pos);
            }
        }
        if (subPos != StringPiece::npos) {
            return start + subPos;
        }
        start = endPos + kEndFlag.size();
    }
}

} // end of namespace logtail