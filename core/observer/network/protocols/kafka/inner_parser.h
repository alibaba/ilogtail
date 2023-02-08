#pragma once

#include "network/protocols/utils.h"
#include "unordered_map"
#include "interface/type.h"

namespace logtail {


enum class KafkaApiType : uint8_t {
    Produce = 0,
    Fetch = 1,
    ListOffsets = 2,
    Metadata = 3,
    LeaderAndIsr = 4,
    StopReplica = 5,
    UpdateMetadata = 6,
    ControlledShutdown = 7,
    OffsetCommit = 8,
    OffsetFetch = 9,
    FindCoordinator = 10,
    JoinGroup = 11,
    Heartbeat = 12,
    LeaveGroup = 13,
    SyncGroup = 14,
    DescribeGroups = 15,
    ListGroups = 16,
    SaslHandshake = 17,
    ApiVersions = 18,
    CreateTopics = 19,
    DeleteTopics = 20,
    DeleteRecords = 21,
    InitProducerId = 22,
    OffsetForLeaderEpoch = 23,
    AddPartitionsToTxn = 24,
    AddOffsetsToTxn = 25,
    EndTxn = 26,
    WriteTxnMarkers = 27,
    TxnOffsetCommit = 28,
    DescribeAcls = 29,
    CreateAcls = 30,
    DeleteAcls = 31,
    DescribeConfigs = 32,
    AlterConfigs = 33,
    AlterReplicaLogDirs = 34,
    DescribeLogDirs = 35,
    SaslAuthenticate = 36,
    CreatePartitions = 37,
    CreateDelegationToken = 38,
    RenewDelegationToken = 39,
    ExpireDelegationToken = 40,
    DescribeDelegationToken = 41,
    DeleteGroups = 42,
    ElectLeaders = 43,
    IncrementalAlterConfigs = 44,
    AlterPartitionReassignments = 45,
    ListPartitionReassignments = 46,
    OffsetDelete = 47,
    DescribeClientQuotas = 48,
    AlterClientQuotas = 49,
    DescribeUserScramCredentials = 50,
    AlterUserScramCredentials = 51,
    AlterIsr = 56,
    UpdateFeatures = 57,
    DescribeCluster = 60,
    DescribeProducers = 61,
};


enum class KafkaMsgType : uint8_t {
    UnknownType = 0,
    ProduceType = 1,
    FetchType = 2,
    // currently, only support measure produce and fetch request
    UnsupportedType = 3,
    IllegalType = 4,
};

inline std::string KafkaMsgTypeToString(KafkaMsgType type) {
    switch (type) {
        case KafkaMsgType::ProduceType:
            return "kafka_produce";
        case KafkaMsgType::FetchType:
            return "kafka_fetch";
        case KafkaMsgType::UnsupportedType:
            return "kafka_unsupported";
        case KafkaMsgType::IllegalType:
            return "kafka_illegal";
        default:
            return "kafka_unknown";
    }
}

inline std::string KafkaApiTypeToString(KafkaApiType type) {
    switch (type) {
        case KafkaApiType::Produce:
            return "produce";
        case KafkaApiType::Fetch:
            return "fetch";
        default:
            return "unknown";
    }
}


struct KafkaRequestData {
    KafkaApiType ApiKey;
    uint16_t Version;
    uint16_t Acks;
    uint32_t TimeoutMs;
    StringPiece ClientId;
    StringPiece Topic;
};

struct KafkaResponseData {
    StringPiece Topic;
    uint16_t PartitionID;
    uint16_t Code;
};

struct KafkaData {
    KafkaMsgType Type;
    uint64_t CorrelationId;
    union {
        KafkaRequestData Request;
        KafkaResponseData Response;
    };
};

class KafkaParser : public ProtoParser {
public:
    KafkaParser(const char* payload, const size_t pktSize) : ProtoParser(payload, pktSize, true) {}
    void ParseRequest();
    void ParseResponse(KafkaApiType apiType, uint16_t version);
    void ParseResponseHeader();

private:
    void parseProduceRequest();
    void parseFetchRequest();
    void parseProduceResponse(uint16_t i);
    void parseFetchResponse(uint16_t i);

    bool readNullableString(StringPiece& piece);
    bool readNullableCompactString(StringPiece& piece);
    bool readString(StringPiece& piece);
    bool readCompactString(StringPiece& piece);
    bool readTags();
    int32_t readArraySize();
    int32_t readCompactArraySize();
    bool isFlexible(KafkaApiType type, uint16_t version);
    bool isSupportedApi(KafkaApiType type, uint16_t version);

public:
    KafkaData mData{};


    friend class ProtocolKafkaUnittest;
};

} // namespace logtail