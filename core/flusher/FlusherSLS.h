#pragma once

#include "flusher/Flusher.h"

#include <cstdint>
#include <string>
#include <vector>

#include "common/LogstoreFeedbackKey.h"

namespace logtail {
class FlusherSLS : public Flusher {
public:
    enum class CompressType { NONE, LZ4, ZSTD };
    enum class TelemetryType { LOG, METRIC };

    struct Batch {
        enum class MergeType { TOPIC, LOGSTORE };

        Batch();

        MergeType mMergeType = MergeType::TOPIC;
        uint32_t mSendIntervalSecs;
        std::vector<std::string> mShardHashKeys;
    };

    FlusherSLS();

    bool Init(const Table& config) override;
    bool Start() override;
    bool Stop(bool isPipelineRemoving) override;

    std::string mProject;
    std::string mLogstore;
    std::string mRegion;
    std::string mEndpoint;
    std::string mAliuid;
    CompressType mCompressType = CompressType::LZ4;
    TelemetryType mTelemetryType = TelemetryType::LOG;
    uint32_t mFlowControlExpireTime = 0;
    int32_t mMaxSendRate = -1;
    Batch mBatch;

private:
    static std::string sName;

    LogstoreFeedBackKey mLogstoreKey;
};
} // namespace logtail