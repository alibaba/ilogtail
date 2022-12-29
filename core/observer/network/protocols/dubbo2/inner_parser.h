#pragma once

#include "network/protocols/utils.h"

namespace logtail {

class DeserializeParser {
public:
    virtual int SkipString(ProtoParser& parser) = 0;
    virtual SlsStringPiece ReadString(ProtoParser& parser) = 0;
};

class Hessian2Parser : public DeserializeParser {
public:
    int SkipString(ProtoParser& parser) override;
    SlsStringPiece ReadString(ProtoParser& parser) override;
};

class FastjsonParser : public DeserializeParser {
public:
    int SkipString(ProtoParser& parser) override;
    SlsStringPiece ReadString(ProtoParser& parser) override;
};

enum class DubboMsgType : uint8_t {
    // illegal packet
    IllegalType = 0,
    // ignore types
    HeartbeatType = 1,
    OnewayType = 2,
    UnsupportProtocolType = 3,
    NormalMsgType = 4,

};


struct DubboRequest {
    SlsStringPiece ReqVersion;
    SlsStringPiece ReqServiceName;
    SlsStringPiece ReqServiceVersionName;
    SlsStringPiece ReqMethodName;
};

struct DubboResponse {
    uint8_t RespStatus;
};
struct DubboData {
    DubboMsgType Type;
    uint64_t ReqId;
    union {
        DubboRequest Request;
        DubboResponse Response;
    };
};

class DubboParser : public ProtoParser {
public:
    DubboParser(const char* payload, const size_t pktSize) : ProtoParser(payload, pktSize, true) {}

    void Parse();

    DeserializeParser* GetDeserializeParser(uint8_t serialId) {
        switch (serialId) {
            case 0x02:
                return &mHessianParser;
            case 0x06:
                return &mFastjsonParser;
            default:
                return nullptr;
        }
    }

public:
    DubboData mData{};

private:
    Hessian2Parser mHessianParser;
    FastjsonParser mFastjsonParser;
};


} // namespace logtail