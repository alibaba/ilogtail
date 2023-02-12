#include "inner_parser.h"
#include "interface/protocol.h"
#include "type.h"


ParseResult logtail::HTTPParser::ParseRequest() {
    int ret = this->parseRequest(mPiece.data(), mPiece.size());
    if (ret == -2) {
        return ParseResult::ParseResult_Partial;
    }
    if (ret >= 0) {
        this->convertRawHeaders();
        mPiece = mPiece.substr(ret);
        mOffset += ret;
        return this->parseRequestBody(mPiece);
    }
    return ParseResult::ParseResult_Fail;
}

ParseResult logtail::HTTPParser::ParseResp() {
    int ret = this->parseResp(mPiece.data(), mPiece.size());
    if (ret == -2) {
        return ParseResult::ParseResult_Partial;
    }
    if (ret >= 0) {
        this->convertRawHeaders();
        mOffset += ret;
        mPiece = mPiece.substr(ret);
        return this->parseResponseBody(mPiece);
    }
    return ParseResult::ParseResult_Fail;
}


int logtail::HTTPParser::parseRequest(const char* buf, size_t size) {
    return phr_parse_request(buf,
                             size,
                             &mPacket.Req.Method,
                             &mPacket.Req.MethodLen,
                             &mPacket.Req.Url,
                             &mPacket.Req.UrlLen,
                             &mPacket.Common.Version,
                             mPacket.Common.RawHeaders,
                             &mPacket.Common.RawHeadersNum,
                             /*last_len*/ 0);
}
int logtail::HTTPParser::parseResp(const char* buf, size_t size) {
    return phr_parse_response(buf,
                              size,
                              &mPacket.Common.Version,
                              &mPacket.Resp.Code,
                              &mPacket.Resp.Msg,
                              &mPacket.Resp.MsgLen,
                              mPacket.Common.RawHeaders,
                              &mPacket.Common.RawHeadersNum,
                              /*last_len*/ 0);
}

ParseResult logtail::HTTPParser::parseRequestBody(StringPiece piece) {
    // content-length
    auto iter = this->mPacket.Common.Headers.find(kContentLength);
    if (iter != this->mPacket.Common.Headers.end()) {
        return this->parseContent(iter->second, piece, &this->mPacket.Resp.Body);
    }
    // chunked
    iter = this->mPacket.Common.Headers.find(kTransferEncoding);
    if (iter != this->mPacket.Common.Headers.end() && iter->second == "chunked") {
        //        return this->parseChunked(StringPiece{buf, size});
        // current do nothing for chunked data.
        return ParseResult_OK;
    }
    return ParseResult::ParseResult_OK;
}
void logtail::HTTPParser::convertRawHeaders() {
    for (size_t i = 0; i < this->mPacket.Common.RawHeadersNum; ++i) {
        this->mPacket.Common.Headers.insert(
            std::make_pair(StringPiece(mPacket.Common.RawHeaders[i].name, mPacket.Common.RawHeaders[i].name_len),
                           StringPiece(mPacket.Common.RawHeaders[i].value, mPacket.Common.RawHeaders[i].value_len)));
    }
}
ParseResult logtail::HTTPParser::parseContent(StringPiece total, StringPiece piece, StringPiece* body) {
    auto originLen = std::strtol(total.data(), NULL, 10);
    if (originLen == 0) {
        return ParseResult_OK;
    }
    size_t len = originLen;
    if (len > 1024) {
        len = 1024;
    }
    if (piece.size() < len) {
        return ParseResult_Partial;
    }
    *body = piece.substr(0, len);
    mOffset += static_cast<int32_t>(originLen); // discard overflow message.
    return ParseResult_OK;
}

ParseResult logtail::HTTPParser::parseResponseBody(logtail::StringPiece piece) {
    auto iter = this->mPacket.Common.Headers.find(kContentLength);
    if (iter != this->mPacket.Common.Headers.end()) {
        return this->parseContent(iter->second, piece, &this->mPacket.Resp.Body);
    }

    iter = this->mPacket.Common.Headers.find(kTransferEncoding);
    if (iter != this->mPacket.Common.Headers.end() && iter->second == "chunked") {
        // current do nothing for chunked data.
        return ParseResult_OK;
    }
    if ((mPacket.Resp.Code >= 100 && mPacket.Resp.Code < 200) || mPacket.Resp.Code == 204 || mPacket.Resp.Code == 304) {
        return ParseResult_OK;
    }
    return ParseResult_Drop;
}
