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

#include <vector>
#include <unordered_map>
#include <iostream>

#include "common/protocol/picohttpparser/picohttpparser.h"
#include "network/protocols/utils.h"
#include "interface/protocol.h"

namespace logtail {

struct HTTPCommonPacket {
    int Version{0};
    size_t RawHeadersNum{50};
    struct phr_header RawHeaders[50]{};
    StringPieceHashMap Headers;
};

struct Request {
    const char* Method{nullptr};
    const char* Url{nullptr};
    size_t MethodLen{0};
    size_t UrlLen{0};
    StringPiece Body{};
};

struct Response {
    const char* Msg{nullptr};
    size_t MsgLen{0};
    int Code{0};
    StringPiece Body{};
};

struct Packet {
    Request Req;
    Response Resp;
    HTTPCommonPacket Common;
};


class HTTPParser {
public:
    HTTPParser(const char* buf, size_t size) : mPiece(buf, size), mOffset(0){};
    ParseResult ParseRequest();
    ParseResult ParseResp();
    const Packet& GetPacket() { return this->mPacket; }
    int32_t GetCostOffset() const { return this->mOffset; };

private:
    int parseRequest(const char* buf, size_t size);
    int parseResp(const char* buf, size_t size);
    ParseResult parseRequestBody(StringPiece piece);
    void convertRawHeaders();
    ParseResult parseContent(StringPiece total, StringPiece piece, StringPiece* body);
    ParseResult parseResponseBody(StringPiece detail);

    StringPiece mPiece;
    Packet mPacket;
    int32_t mOffset;
};

} // end of namespace logtail