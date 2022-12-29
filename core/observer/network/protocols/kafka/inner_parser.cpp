#include "inner_parser.h"

struct KafkaSupporttedVersion {
    uint8_t minVersion;
    uint8_t maxVersion;
    int16_t flexibleVersion;
};

static const std::unordered_map<uint8_t, KafkaSupporttedVersion> versions{
    {static_cast<uint8_t>(logtail::KafkaApiType::Produce), {1, 9, 9}},
    {static_cast<uint8_t>(logtail::KafkaApiType::Fetch), {0, 12, 12}},
    {static_cast<uint8_t>(logtail::KafkaApiType::ListOffsets), {0, 7, 6}},
    {static_cast<uint8_t>(logtail::KafkaApiType::Metadata), {0, 12, 9}},
    {static_cast<uint8_t>(logtail::KafkaApiType::LeaderAndIsr), {0, 5, 4}},
    {static_cast<uint8_t>(logtail::KafkaApiType::StopReplica), {0, 3, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::UpdateMetadata), {0, 7, 6}},
    {static_cast<uint8_t>(logtail::KafkaApiType::ControlledShutdown), {0, 3, 3}},
    {static_cast<uint8_t>(logtail::KafkaApiType::OffsetCommit), {0, 8, 8}},
    {static_cast<uint8_t>(logtail::KafkaApiType::OffsetFetch), {0, 8, 6}},
    {static_cast<uint8_t>(logtail::KafkaApiType::FindCoordinator), {0, 4, 3}},
    {static_cast<uint8_t>(logtail::KafkaApiType::JoinGroup), {0, 7, 6}},
    {static_cast<uint8_t>(logtail::KafkaApiType::Heartbeat), {0, 4, 4}},
    {static_cast<uint8_t>(logtail::KafkaApiType::LeaveGroup), {0, 4, 4}},
    {static_cast<uint8_t>(logtail::KafkaApiType::SyncGroup), {0, 5, 4}},
    {static_cast<uint8_t>(logtail::KafkaApiType::DescribeGroups), {0, 5, 5}},
    {static_cast<uint8_t>(logtail::KafkaApiType::ListGroups), {0, 4, 3}},
    {static_cast<uint8_t>(logtail::KafkaApiType::SaslHandshake), {0, 1, -1}},
    {static_cast<uint8_t>(logtail::KafkaApiType::ApiVersions), {0, 3, 3}},
    {static_cast<uint8_t>(logtail::KafkaApiType::CreateTopics), {0, 7, 5}},
    {static_cast<uint8_t>(logtail::KafkaApiType::DeleteTopics), {0, 6, 4}},
    {static_cast<uint8_t>(logtail::KafkaApiType::DeleteRecords), {0, 2, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::InitProducerId), {0, 4, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::OffsetForLeaderEpoch), {0, 4, 4}},
    {static_cast<uint8_t>(logtail::KafkaApiType::AddPartitionsToTxn), {0, 3, 3}},
    {static_cast<uint8_t>(logtail::KafkaApiType::AddOffsetsToTxn), {0, 3, 3}},
    {static_cast<uint8_t>(logtail::KafkaApiType::EndTxn), {0, 3, 3}},
    {static_cast<uint8_t>(logtail::KafkaApiType::WriteTxnMarkers), {0, 1, 1}},
    {static_cast<uint8_t>(logtail::KafkaApiType::TxnOffsetCommit), {0, 3, 3}},
    {static_cast<uint8_t>(logtail::KafkaApiType::DescribeAcls), {0, 2, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::CreateAcls), {0, 2, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::DeleteAcls), {0, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::DescribeConfigs), {0, 4, 4}},
    {static_cast<uint8_t>(logtail::KafkaApiType::AlterConfigs), {0, 2, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::AlterReplicaLogDirs), {0, 2, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::DescribeLogDirs), {0, 2, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::SaslAuthenticate), {0, 2, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::CreatePartitions), {0, 3, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::CreateDelegationToken), {0, 2, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::RenewDelegationToken), {0, 2, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::ExpireDelegationToken), {0, 2, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::DescribeDelegationToken), {0, 2, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::DeleteGroups), {0, 2, 5}},
    {static_cast<uint8_t>(logtail::KafkaApiType::ElectLeaders), {0, 2, 2}},
    {static_cast<uint8_t>(logtail::KafkaApiType::IncrementalAlterConfigs), {0, 1, 1}},
    {static_cast<uint8_t>(logtail::KafkaApiType::AlterPartitionReassignments), {0, 0, 0}},
    {static_cast<uint8_t>(logtail::KafkaApiType::ListPartitionReassignments), {0, 0, 0}},
    {static_cast<uint8_t>(logtail::KafkaApiType::OffsetDelete), {0, 0, -1}},
    {static_cast<uint8_t>(logtail::KafkaApiType::DescribeClientQuotas), {0, 1, 1}},
    {static_cast<uint8_t>(logtail::KafkaApiType::AlterClientQuotas), {0, 1, 1}},
    {static_cast<uint8_t>(logtail::KafkaApiType::DescribeUserScramCredentials), {0, 0, 0}},
    {static_cast<uint8_t>(logtail::KafkaApiType::AlterUserScramCredentials), {0, 0, 0}},
    {static_cast<uint8_t>(logtail::KafkaApiType::AlterIsr), {0, 0, 0}},
    {static_cast<uint8_t>(logtail::KafkaApiType::UpdateFeatures), {0, 0, 0}},
    {static_cast<uint8_t>(logtail::KafkaApiType::DescribeCluster), {0, 0, 0}},
    {static_cast<uint8_t>(logtail::KafkaApiType::DescribeProducers), {0, 0, 0}},
};


#define READ_STRING(name) \
    do { \
        this->mData.Request.Req##name = parser->ReadString(*this); \
        if (!this->OK()) { \
            this->mData.Type = DubboMsgType::IllegalType; \
            return; \
        } \
    } while (0)

#define CONFIG_TYPE_RETURN(name) \
    do { \
        this->mData.Type = KafkaMsgType::name; \
        return; \
    } while (0)

void logtail::KafkaParser::parseProduceRequest() {
    bool compact = this->mData.Request.Version >= 9;
    if (this->mData.Request.Version >= 3) {
        SlsStringPiece piece; // transactionId
        compact ? readNullableCompactString(piece) : readNullableString(piece);
    }
    this->mData.Request.Acks = readUint16();
    this->mData.Request.TimeoutMs = readUint32();
    uint32_t size = compact ? readCompactArraySize() : readArraySize();
    if (size == -1) {
        CONFIG_TYPE_RETURN(IllegalType);
    }
    if (size > 0) {
        SlsStringPiece piece;
        bool success = compact ? readCompactString(piece) : readString(piece);
        if (success) {
            this->mData.Request.Topic = piece;
            return;
        }
        CONFIG_TYPE_RETURN(IllegalType);
    }
}
void logtail::KafkaParser::parseFetchRequest() {
    bool compact = this->mData.Request.Version >= 12;
    this->positionCommit(12);
    if (this->mData.Request.Version >= 3) {
        this->positionCommit(4);
    }
    if (this->mData.Request.Version >= 4) {
        this->positionCommit(1);
    }
    if (this->mData.Request.Version >= 7) {
        this->positionCommit(8);
    }
    uint32_t size = compact ? readCompactArraySize() : readArraySize();
    if (size > 0) {
        SlsStringPiece piece;
        bool success = compact ? readCompactString(piece) : readString(piece);
        if (success) {
            this->mData.Request.Topic = piece;
        }
    }
}

// https://kafka.apache.org/protocol.html#The_Messages_Produce
void logtail::KafkaParser::parseProduceResponse(uint16_t version) {
    bool compact = isFlexible(KafkaApiType::Produce, version);
    uint32_t size = compact ? readCompactArraySize() : readArraySize();
    if (size >= 0) {
        SlsStringPiece piece;
        bool success = compact ? readCompactString(piece) : readString(piece);
        if (!success) {
            CONFIG_TYPE_RETURN(IllegalType);
        }
        this->mData.Response.Topic = piece;
        uint32_t partitionNum = compact ? readCompactArraySize() : readArraySize();
        this->mData.Response.PartitionID = partitionNum;
        this->mData.Response.Code = 0;
        if (partitionNum > 0) {
            this->positionCommit(4);
            uint16_t i = readUint16(true);
            if (!OK()) {
                CONFIG_TYPE_RETURN(IllegalType);
            }
            this->mData.Response.Code = i;
        }
    }
}
void logtail::KafkaParser::parseFetchResponse(uint16_t version) {
    bool compact = isFlexible(KafkaApiType::Fetch, version);
    if (version >= 1) {
        this->positionCommit(4); // throttle_time_ms
    }
    this->mData.Response.Code = 0;
    if (version >= 7) {
        uint16_t i = readUint16(true);
        if (!OK()) {
            CONFIG_TYPE_RETURN(IllegalType);
        }
        this->mData.Response.Code = i;
        this->positionCommit(4);
    }
    uint32_t size = compact ? readCompactArraySize() : readArraySize();
    if (size > 0) {
        SlsStringPiece piece;
        bool success = compact ? readCompactString(piece) : readString(piece);
        this->mData.Response.Topic = piece;
        if (!success) {
            CONFIG_TYPE_RETURN(IllegalType);
        }
        if (version < 7) {
            uint32_t partitionNum = compact ? readCompactArraySize() : readArraySize();
            if (partitionNum > 0) {
                this->mData.Response.PartitionID = readUint32();
                uint16_t i = readUint16();
                if (!OK()) {
                    CONFIG_TYPE_RETURN(IllegalType);
                }
                this->mData.Response.Code = i;
            }
        }
    }
}

bool logtail::KafkaParser::readNullableString(logtail::SlsStringPiece& piece) {
    if (this->getLeftSize() < 2) {
        return false;
    }
    auto len = static_cast<int16_t>(this->readUint16(false));
    if (len < -1) {
        return false;
    }
    if (len == -1) {
        this->positionCommit(2);
        piece = SlsStringPiece{};
        return true;
    }
    this->positionCommit(2);
    piece = SlsStringPiece(this->head(), len);
    this->positionCommit(len);
    return true;
}
bool logtail::KafkaParser::readNullableCompactString(logtail::SlsStringPiece& piece) {
    int64_t len = readVarInt64(true);
    len -= 1;
    if (len < -1) {
        return false;
    }
    if (len == -1) {
        piece = SlsStringPiece{};
        return true;
    }
    piece = SlsStringPiece(this->head(), len);
    this->positionCommit(len);
    return true;
}
uint32_t logtail::KafkaParser::readArraySize() {
    if (this->getLeftSize() < 2) {
        return -1;
    }
    auto size = static_cast<int32_t>(readUint32(false));
    if (size < -1) {
        return -1;
    }
    this->positionCommit(4);
    return size == -1 ? 0 : size;
}

uint32_t logtail::KafkaParser::readCompactArraySize() {
    int64_t len = readVarInt64(true);
    if (len < 0) {
        return -1;
    } else if (len == 0) {
        return 0;
    } else {
        return len - 1;
    }
}
bool logtail::KafkaParser::readString(logtail::SlsStringPiece& piece) {
    if (this->getLeftSize() < 2) {
        return false;
    }
    auto len = static_cast<int16_t>(this->readUint16(false));
    if (len < 0) {
        return false;
    }
    this->positionCommit(2);
    piece = SlsStringPiece(this->head(), len);
    this->positionCommit(len);
    return true;
}
bool logtail::KafkaParser::readCompactString(logtail::SlsStringPiece& piece) {
    int64_t len = readVarInt64(true);
    len -= 1;
    if (len < 0) {
        return false;
    }
    piece = SlsStringPiece(this->head(), len);
    this->positionCommit(len);
    return true;
}
void logtail::KafkaParser::ParseRequest() {
    uint32_t len = readUint32();
    if (len < 16 || len > this->getLeftSize()) {
        CONFIG_TYPE_RETURN(IllegalType);
    }
    uint16_t apiKey = readUint16();
    uint16_t apiVersion = readUint16();
    if (!isSupportedApi(static_cast<KafkaApiType>(apiKey), apiVersion)) {
        CONFIG_TYPE_RETURN(IllegalType);
    }
    this->mData.Request.Version = apiVersion;
    this->mData.Request.ApiKey = static_cast<KafkaApiType>(apiKey);
    this->mData.CorrelationId = readUint32();
    uint16_t clientIdLen = readUint16();
    if (clientIdLen > this->getLeftSize()) {
        CONFIG_TYPE_RETURN(IllegalType);
    }
    this->mData.Request.ClientId = SlsStringPiece{this->head(), clientIdLen};
    this->positionCommit(clientIdLen);
    if (isFlexible(static_cast<KafkaApiType>(apiKey), apiVersion)) {
        readTags();
    }
    switch (this->mData.Request.ApiKey) {
        case KafkaApiType::Produce:
            this->mData.Type = KafkaMsgType::ProduceType;
            parseProduceRequest();
            break;
        case KafkaApiType::Fetch:
            this->mData.Type = KafkaMsgType::FetchType;
            parseFetchRequest();
            break;
        default:
            CONFIG_TYPE_RETURN(UnsupportedType);
    }
}
void logtail::KafkaParser::ParseResponse(logtail::KafkaApiType apiType, uint16_t version) {
    if (isFlexible(apiType, version)) {
        readTags();
    }
    switch (apiType) {
        case KafkaApiType::Produce:
            this->mData.Type = KafkaMsgType::ProduceType;
            parseProduceResponse(version);
            break;
        case KafkaApiType::Fetch:
            this->mData.Type = KafkaMsgType::FetchType;
            parseFetchResponse(version);
            break;
        default:
            CONFIG_TYPE_RETURN(UnsupportedType);
    }
}

void logtail::KafkaParser::ParseResponseHeader() {
    uint32_t len = readUint32();
    if (len < 8) {
        CONFIG_TYPE_RETURN(IllegalType);
    }
    this->mData.CorrelationId = readUint32();
}
bool logtail::KafkaParser::readTags() {
    // todo: currently, only parse but not send anything.
    int64_t num = readVarInt32();
    for (int i = 0; i < num; ++i) {
        int64_t tag = readVarInt64();
        int64_t len = readVarInt64();
        SlsStringPiece(this->head(), len);
        this->positionCommit(len);
    }
    return true;
}
inline bool logtail::KafkaParser::isSupportedApi(KafkaApiType type, uint16_t version) {
    auto iter = versions.find(static_cast<uint8_t>(type));
    if (iter == versions.end()) {
        return false;
    }
    if (version < iter->second.minVersion || version > iter->second.maxVersion) {
        return false;
    }
    return true;
}
inline bool logtail::KafkaParser::isFlexible(KafkaApiType type, uint16_t version) {
    if (isSupportedApi(type, version)) {
        auto iter = versions.find(static_cast<uint8_t>(type));
        return iter->second.flexibleVersion <= version;
    }
    return false;
}
