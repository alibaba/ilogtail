#pragma once

#include "network/protocols/utils.h"
#include "StringPiece.h"

namespace logtail {

class DeserializeParser {
public:
    virtual int SkipString(ProtoParser& parser) = 0;
    virtual StringPiece ReadString(ProtoParser& parser) = 0;
};

class Hessian2Parser : public DeserializeParser {
public:
    int SkipString(ProtoParser& parser) override;
    StringPiece ReadString(ProtoParser& parser) override;
};

class FastjsonParser : public DeserializeParser {
public:
    int SkipString(ProtoParser& parser) override;
    StringPiece ReadString(ProtoParser& parser) override;
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
    StringPiece ReqVersion;
    StringPiece ReqServiceName;
    StringPiece ReqServiceVersionName;
    StringPiece ReqMethodName;
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