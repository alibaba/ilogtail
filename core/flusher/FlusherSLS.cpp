#include "flusher/FlusherSLS.h"

#include "app_config/AppConfig.h"
#include "common/ParamExtractor.h"
#include "sender/Sender.h"

using namespace std;

DECLARE_FLAG_INT32(batch_send_interval);
DEFINE_FLAG_BOOL(sls_client_send_compress, "whether compresses the data or not when put data", true);

namespace logtail {
string FlusherSLS::sName = "flusher_SLS";

FlusherSLS::FlusherSLS() : mRegion(AppConfig::GetInstance()->GetDefaultRegion()) {
}

FlusherSLS::Batch::Batch() : mSendIntervalSecs(INT32_FLAG(batch_send_interval)) {
}

bool FlusherSLS::Init(const Table& config) {
    Json::Value config1;
    string errorMsg;

    // Project
    if (!GetMandatoryStringParam(config1, "Project", mProject, errorMsg)) {
        PARAM_ERROR(sLogger, errorMsg, sName, mContext->GetConfigName());
    }

    // Logstore
    if (!GetMandatoryStringParam(config1, "Logstore", mLogstore, errorMsg)) {
        PARAM_ERROR(sLogger, errorMsg, sName, mContext->GetConfigName());
    }
    mLogstoreKey = GenerateLogstoreFeedBackKey(mProject, mLogstore);

    // Region
    if (!GetOptionalStringParam(config1, "Region", mRegion, errorMsg)) {
        PARAM_WARNING_DEFAULT(
            sLogger, errorMsg, AppConfig::GetInstance()->GetDefaultRegion(), sName, mContext->GetConfigName());
    }
#ifdef __ENTERPRISE__
    if (AppConfig::GetInstance()->IsDataServerPrivateCloud()) {
        mRegion = STRING_FLAG(default_region_name);
    }
#endif

    // Endpoint
    if (!GetMandatoryStringParam(config1, "Endpoint", mEndpoint, errorMsg)) {
        PARAM_ERROR(sLogger, errorMsg, sName, mContext->GetConfigName());
    }
    if (!mEndpoint.empty()) {
        Sender::Instance()->AddEndpointEntry(mRegion, CheckAddress(mEndpoint, mEndpoint));
    }

    // Aliuid
    if (!GetOptionalStringParam(config1, "Aliuid", mAliuid, errorMsg)) {
        PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
    }

    // CompressType
    string compressType;
    if (!GetOptionalStringParam(config1, "CompressType", compressType, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, "lz4", sName, mContext->GetConfigName());
    }
    if (BOOL_FLAG(sls_client_send_compress)) {
        if (compressType == "zstd") {
            mCompressType = CompressType::ZSTD;
        } else if (compressType == "none") {
            mCompressType = CompressType::NONE;
        } else if (compressType != "lz4") {
            PARAM_WARNING_DEFAULT(sLogger, "param CompressType is not valid", "lz4", sName, mContext->GetConfigName());
        }
    } else {
        mCompressType = CompressType::NONE;
    }

    // TelemetryType
    string telemetryType;
    if (!GetOptionalStringParam(config1, "TelemetryType", telemetryType, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, "logs", sName, mContext->GetConfigName());
    }
    if (telemetryType == "metrics") {
        mTelemetryType = TelemetryType::METRIC;
    } else if (telemetryType != "logs") {
        PARAM_WARNING_DEFAULT(sLogger, "param TelemetryType is not valid", "logs", sName, mContext->GetConfigName());
    }

    // FlowControlExpireTime
    if (!GetOptionalUIntParam(config1, "FlowControlExpireTime", mFlowControlExpireTime, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, 0, sName, mContext->GetConfigName());
    }

    // MaxSendRate
    if (!GetOptionalIntParam(config1, "FlowControlExpireTime", mMaxSendRate, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, -1, sName, mContext->GetConfigName());
    }
    Sender::Instance()->SetLogstoreFlowControl(mLogstoreKey, mMaxSendRate, mFlowControlExpireTime);

    // Batch
    const char* key = "Batch";
    const Json::Value* itr = config1.find(key, key + strlen(key));
    if (itr && itr->isObject()) {
        // MergeType
        string mergeType;
        if (!GetOptionalStringParam(*itr, "Batch.MergeType", mergeType, errorMsg)) {
            PARAM_WARNING_DEFAULT(sLogger, errorMsg, "topic", sName, mContext->GetConfigName());
        }
        if (mergeType == "logstore") {
            mBatch.mMergeType = Batch::MergeType::LOGSTORE;
        } else if (mergeType != "topic") {
            PARAM_WARNING_DEFAULT(
                sLogger, "param Batch.MergeType is not valid", "topic", sName, mContext->GetConfigName());
        }

        // SendIntervalSecs
        if (!GetOptionalUIntParam(*itr, "Batch.SendIntervalSecs", mBatch.mSendIntervalSecs, errorMsg)) {
            PARAM_WARNING_DEFAULT(sLogger, errorMsg, INT32_FLAG(batch_send_interval), sName, mContext->GetConfigName());
        }

        // ShardHashKeys
        if (!GetOptionalListParam<string>(*itr, "Batch.ShardHashKeys", mBatch.mShardHashKeys, errorMsg)) {
            PARAM_WARNING_IGNORE(sLogger, errorMsg, sName, mContext->GetConfigName());
        }
    }

    return true;
}

bool FlusherSLS::Start() {
    Sender::Instance()->IncreaseProjectReferenceCnt(mProject);
    Sender::Instance()->IncreaseRegionReferenceCnt(mRegion);
    Sender::Instance()->IncreaseAliuidReferenceCntForRegion(mRegion, mAliuid);
    return true;
}

bool FlusherSLS::Stop(bool isPipelineRemoving) {
    Sender::Instance()->DecreaseProjectReferenceCnt(mProject);
    Sender::Instance()->DecreaseRegionReferenceCnt(mRegion);
    Sender::Instance()->DecreaseAliuidReferenceCntForRegion(mRegion, mAliuid);
    return true;
}
} // namespace logtail