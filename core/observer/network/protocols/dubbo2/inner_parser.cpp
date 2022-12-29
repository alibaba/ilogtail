#include "inner_parser.h"


static const uint8_t kSerialMask = 0x1f;
static const uint8_t kEventFlag = 0x20;
static const uint8_t kTwoWayFlag = 0x40;
static const uint8_t kReqFlag = 0x80;

#define READ_STRING(name) \
    do { \
        this->mData.Request.Req##name = parser->ReadString(*this); \
        if (!this->OK()) { \
            this->mData.Type = DubboMsgType::IllegalType; \
            return; \
        } \
    } while (0)
void logtail::DubboParser::Parse() {
    if (this->getLeftSize() < 16) {
        this->mData.Type = DubboMsgType::IllegalType;
        return;
    }
    this->mData.Type = DubboMsgType::NormalMsgType;
    // skip magic & req/resp bit because they were used to detect protocol.
    this->positionCommit(2);
    uint8_t c = readUint8();
    if (c & kEventFlag) {
        this->mData.Type = DubboMsgType::HeartbeatType;
        return;
    }
    bool req = static_cast<uint8_t>(c & kReqFlag);
    if (req && !(c & kTwoWayFlag)) {
        this->mData.Type = DubboMsgType::OnewayType;
        return;
    }
    auto serialId = static_cast<uint8_t>(c & kSerialMask);
    if (!serialId) {
        this->mData.Type = DubboMsgType::IllegalType;
        return;
    }
    auto parser = GetDeserializeParser(serialId);
    if (parser == nullptr) {
        this->mData.Type = DubboMsgType::UnsupportProtocolType;
        return;
    }

    uint8_t status = readUint8();
    if (!req) {
        this->mData.Response.RespStatus = status;
    }
    this->mData.ReqId = readUint64();
    if (!req) {
        return;
    }
    uint32_t len = readUint32();
    if (len > static_cast<uint32_t>(this->getLeftSize())) {
        this->mData.Type = DubboMsgType::IllegalType;
        return;
    }
    READ_STRING(Version);
    READ_STRING(ServiceName);
    READ_STRING(ServiceVersionName);
    READ_STRING(MethodName);
}
int logtail::Hessian2Parser::SkipString(logtail::ProtoParser& parser) {
    uint8_t tag = parser.readUint8();
    if (tag >= 0x30 && tag <= 0x33) {
        uint8_t offset = parser.readUint8();
        if (!parser.OK()) {
            return -1;
        }
        parser.positionCommit(((tag - 0x30) << 8) + offset);
        if (!parser.OK()) {
            return -1;
        }
    } else {
        parser.positionCommit(tag);
        if (!parser.OK()) {
            return -1;
        }
    }
    return 0;
}
logtail::SlsStringPiece logtail::Hessian2Parser::ReadString(logtail::ProtoParser& parser) {
    uint8_t tag = parser.readUint8();
    if (tag >= 0x30 && tag <= 0x33) {
        uint8_t offset = parser.readUint8();
        if (!parser.OK()) {
            return SlsStringPiece{};
        }
        uint8_t len = ((tag - 0x30) << 8) + offset;
        const char* head = parser.head();
        parser.positionCommit(len);
        if (!parser.OK()) {
            return SlsStringPiece{};
        }
        return {head, len};
    } else {
        const char* head = parser.head();
        parser.positionCommit(tag);
        if (!parser.OK()) {
            return SlsStringPiece{};
        }
        return {head, tag};
    }
}
int logtail::FastjsonParser::SkipString(logtail::ProtoParser& parser) {
    parser.readUntil(0x0a);
    parser.positionCommit(1);
    if (!parser.OK()) {
        return -1;
    }
    return 0;
}
logtail::SlsStringPiece logtail::FastjsonParser::ReadString(logtail::ProtoParser& parser) {
    const SlsStringPiece& piece = parser.readUntil(0x0a);
    if (!parser.OK()) {
        return SlsStringPiece{};
    }
    return SlsStringPiece{piece.mPtr + 1, piece.mLen - 2};
}
