#pragma once

#include "buffer.h"

namespace logtail {
class Parser {
public:
    virtual ~Parser() = default;

    BufferResult
    AddBuffer(MessageType msgType, int32_t pos, uint64_t time, const char* pkt, int32_t pktSize, int32_t pktRealSize) {
        switch (msgType) {
            case MessageType_Request:
                return mReqBuffer.Add(pos, time, pkt, pktSize, pktRealSize);
            case MessageType_Response:
                return mRespBuffer.Add(pos, time, pkt, pktSize, pktRealSize);
            default:
                return BufferResult_Drop;
        }
    }

    StringPiece GetPiece(MessageType msgType) {
        switch (msgType) {
            case MessageType_Response:
                return mRespBuffer.Head();
            case MessageType_Request:
                return mReqBuffer.Head();
            default:
                return {};
        }
    }

    void RemovePrefix(MessageType msgType, int32_t len) {
        switch (msgType) {
            case MessageType_Response: {
                mRespBuffer.RemovePrefix(len);
                break;
            }
            case MessageType_Request: {
                mReqBuffer.RemovePrefix(len);
                break;
            }
            default:
                return;
        }
    }


    ParseResult OnData(const PacketEventHeader* header, const PacketEventData* data) {
        auto res = AddBuffer(data->MsgType, data->Pos, header->TimeNano, data->Buffer, data->BufferLen, data->RealLen);
        if (res == BufferResult_Drop) {
            return ParseResult_Drop;
        }
        if (res == BufferResult_DirectParse) {
            return OnPacket(data->PktType, data->MsgType, header, data->Buffer, data->BufferLen, data->RealLen, 0);
        }
        auto piece = GetPiece(data->MsgType);
        int32_t offset = 0;
        for (int i = 0; i < 2; ++i) {
            auto parseRes
                = OnPacket(data->PktType, data->MsgType, header, piece.data(), piece.size(), piece.size(), &offset);
            switch (parseRes) {
                case ParseResult_Partial: {
                    return ParseResult_Partial;
                }
                case ParseResult_OK: {
                    RemovePrefix(data->MsgType, offset);
                    return ParseResult_OK;
                }
                case ParseResult_Fail: {
                    auto pos = FindBoundary(data->MsgType, piece);
                    if (pos == std::string::npos) {
                        // means the whole data is invalid, don't need to reparse again.
                        RemovePrefix(data->MsgType, piece.size());
                        return ParseResult_Fail;
                    }
                    offset += static_cast<int32_t>(pos);
                    if (i == 1) {
                        // means the recorrect data fail again, directly to drop;
                        RemovePrefix(data->MsgType, piece.size());
                        return ParseResult_Fail;
                    }
                    break;
                }
                default: {
                    RemovePrefix(data->MsgType, piece.size());
                    return ParseResult_Drop;
                }
            }
        }
        return ParseResult_OK;
    }

    // offset means the start char position, and the used char count should add up to the offset.
    virtual ParseResult OnPacket(PacketType pktType,
                                 MessageType msgType,
                                 const PacketEventHeader* header,
                                 const char* pkt,
                                 int32_t pktSize,
                                 int32_t pktRealSize,
                                 int32_t* offset)
        = 0;

    // return std::string::npos means not found boundary
    virtual size_t FindBoundary(MessageType message_type, const StringPiece& piece) = 0;

    // GC，把内部没有完成Event匹配的消息按照SizeLimit和TimeOut进行清理
    // 返回值，如果是true代表还有数据，false代表无数据
    bool GarbageCollection(size_t size_limit_bytes, uint64_t expireTimeNs) {
        bool req = this->mReqBuffer.GarbageCollection(expireTimeNs);
        bool resp = this->mRespBuffer.GarbageCollection(expireTimeNs);
        return req && resp;
    }

    virtual size_t GetCacheSize() = 0;

protected:
    Buffer mReqBuffer;
    Buffer mRespBuffer;
};
} // namespace logtail
