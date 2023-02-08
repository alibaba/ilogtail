
#include "parser.h"
#include "inner_parser.h"

int32_t logtail::DubboProtocolParser::GetCacheSize() {
    return mCache.GetRequestsSize() + mCache.GetResponsesSize();
}
bool logtail::DubboProtocolParser::GarbageCollection(size_t size_limit_bytes, uint64_t expireTimeNs) {
    return mCache.GarbageCollection(expireTimeNs);
}
ParseResult logtail::DubboProtocolParser::OnPacket(PacketType pktType,
                                                   MessageType msgType,
                                                   PacketEventHeader* header,
                                                   const char* pkt,
                                                   int32_t pktSize,
                                                   int32_t pktRealSize) {
    DubboParser dubbo(pkt, pktSize);
    LOG_TRACE(sLogger,
              ("message_type", MessageTypeToString(msgType))("dubbo data", charToHexString(pkt, pktSize, pktSize)));
    try {
        dubbo.Parse();
    } catch (const std::runtime_error& re) {
        LOG_DEBUG(sLogger,
                  ("dubbo_parse_fail", re.what())("data", charToHexString(pkt, pktSize, pktSize))(
                      "srcPort", header->SrcPort)("dstPort", header->DstPort));
        return ParseResult_Fail;
    } catch (const std::exception& ex) {
        LOG_DEBUG(sLogger,
                  ("dubbo_parse_fail", ex.what())("data", charToHexString(pkt, pktSize, pktSize))(
                      "srcPort", header->SrcPort)("dstPort", header->DstPort));
        return ParseResult_Fail;
    } catch (...) {
        LOG_DEBUG(
            sLogger,
            ("dubbo_parse_fail", "Unknown failure occurred when parse")("data", charToHexString(pkt, pktSize, pktSize))(
                "srcPort", header->SrcPort)("dstPort", header->DstPort));
        return ParseResult_Fail;
    }

    if (dubbo.OK()) {
        bool insertSuccess = true;
        if (msgType == MessageType_Request) {
            insertSuccess = mCache.InsertReq(dubbo.mData.ReqId, [&](DubboRequestInfo* info) {
                info->TimeNano = header->TimeNano;
                info->ReqBytes = pktRealSize;
                info->Service = std::string(dubbo.mData.Request.ReqServiceName.data(),dubbo.mData.Request.ReqServiceName.size());
                info->ServiceVersion = std::string(dubbo.mData.Request.ReqServiceVersionName.data(),dubbo.mData.Request.ReqServiceVersionName.size());
                info->Version = std::string(dubbo.mData.Request.ReqVersion.data(),dubbo.mData.Request.ReqVersion.size());
                info->Method = std::string(dubbo.mData.Request.ReqMethodName.data(),dubbo.mData.Request.ReqMethodName.size());
                LOG_TRACE(sLogger, ("dubbo insert req", info->ToString()));
            });
        } else if (msgType == MessageType_Response) {
            insertSuccess = mCache.InsertResp(dubbo.mData.ReqId, [&](DubboResponseInfo* info) {
                info->TimeNano = header->TimeNano;
                info->RespBytes = pktRealSize;
                info->RespCode = dubbo.mData.Response.RespStatus;
                LOG_TRACE(sLogger, ("dubbo insert resp", info->ToString()));
            });
        }
        return insertSuccess ? ParseResult_OK : ParseResult_Drop;
    }
    return ParseResult_Fail;
}
