//
// Created by lurious on 2024/6/17.
//
#include "flusher/FlusherArmsMetrics.h"

#include "batch/FlushStrategy.h"
#include "common/EndpointUtil.h"
#include "common/HashUtil.h"
#include "common/LogtailCommonFlags.h"
#include "common/ParamExtractor.h"
#include "compression/CompressorFactory.h"
#include "logger/Logger.h"
#include "pipeline/Pipeline.h"
#include "queue/SenderQueueItem.h"
#include "sdk/Common.h"
#include "sender/PackIdManager.h"
#include "sender/SLSClientManager.h"
#include "sender/Sender.h"
#include "serializer/ArmsSerializer.h"

using namespace std;
DEFINE_FLAG_INT32(arms_metrics_batch_send_interval, "batch sender interval (second)(default 3)", 3);
DEFINE_FLAG_INT32(arms_metrics_merge_count_limit, "log count in one logGroup at most", 40000000);
DEFINE_FLAG_INT32(arms_metrics_batch_send_metric_size,
                  "batch send metric size limit(bytes)(default 256KB)",
                  256 * 1024);

namespace logtail {

DEFINE_FLAG_INT32(arms_test_merged_buffer_interval, "default flush merged buffer, seconds", 2);


const string FlusherArmsMetrics::sName = "flusher_arms_metrics";

FlusherArmsMetrics::FlusherArmsMetrics() : mRegion(Sender::Instance()->GetDefaultRegion()) {
    LOG_INFO(sLogger, ("start test arms metrics flusher", "CREATE by lurious"));
}

bool FlusherArmsMetrics::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    mRegion = "cn-hangzhou";
    string errorMsg;
    // Region
    if (!GetOptionalStringParam(config, "Region", mRegion, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mRegion,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }
    // licenseKey
    if (!GetOptionalStringParam(config, "Licensekey", licenseKey, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mRegion,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }
    // pushAppId
    if (!GetOptionalStringParam(config, "PushAppId", pushAppId, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mRegion,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }
    mCompressor = CompressorFactory::GetInstance()->Create(config, *mContext, sName, CompressType::SNAPPY);
    //
    mGroupListSerializer = make_unique<ArmsMetricsEventGroupListSerializer>(this);

    DefaultFlushStrategyOptions strategy{static_cast<uint32_t>(INT32_FLAG(arms_metrics_batch_send_metric_size)),
                                         static_cast<uint32_t>(INT32_FLAG(arms_metrics_merge_count_limit)),
                                         static_cast<uint32_t>(INT32_FLAG(arms_metrics_batch_send_interval))};
    if (!mBatcher.Init(Json::Value(), this, strategy, true)) {
        // when either exactly once is enabled or ShardHashKeys is not empty, we don't enable group batch
        LOG_WARNING(sLogger, ("mBatcher init info: ", "init err!"));
        return false;
    }

    auto mLogstoreKey = GenerateLogstoreFeedBackKey("arms_metric_proj", "arms_metric_proj");
    mSenderQueue = Sender::Instance()->GetSenderQueue(mLogstoreKey);
    LOG_INFO(sLogger, ("init info: ", "arms metrics init successful !"));

    return true;
}

bool FlusherArmsMetrics::Register() {
    Sender::Instance()->IncreaseProjectReferenceCnt(mProject);
    Sender::Instance()->IncreaseRegionReferenceCnt(mRegion);
    SLSClientManager::GetInstance()->IncreaseAliuidReferenceCntForRegion(mRegion, mAliuid);
    return true;
}
bool FlusherArmsMetrics::Unregister(bool isPipelineRemoving) {
    Sender::Instance()->DecreaseProjectReferenceCnt(mProject);
    Sender::Instance()->DecreaseRegionReferenceCnt(mRegion);
    SLSClientManager::GetInstance()->DecreaseAliuidReferenceCntForRegion(mRegion, mAliuid);
    return true;
}


void FlusherArmsMetrics::Send(PipelineEventGroup&& g) {
    if (g.IsReplay()) {
        LOG_WARNING(sLogger, (" IsReplay ", "true do not serialize data!"));
    } else {
        vector<BatchedEventsList> res;
        // mBatcher.Add(std::move(g), res);
        auto batchList = ConvertToBatchedList(std::move(g));
        res.emplace_back(std::move(batchList));
        SerializeAndPush(std::move(res));
    }
}
BatchedEventsList FlusherArmsMetrics::ConvertToBatchedList(PipelineEventGroup&& group) {
    // EventsContainer &&events, SizedMap &&tags, std::shared_ptr<SourceBuffer>&&sourceBuffer, StringView packIdPrefix,
    //     RangeCheckpointPtr &&eoo
    std::vector<BatchedEvents> batchedEventList;
    BatchedEvents batch(std::move(group.MutableEvents()),
                        std::move(group.GetSizedTags()),
                        std::move(group.GetSourceBuffer()),
                        group.GetMetadata(EventGroupMetaKey::SOURCE_ID),
                        std::move(group.GetExactlyOnceCheckpoint()));
    batchedEventList.emplace_back(std::move(batch));
    return batchedEventList;
}
void FlusherArmsMetrics::Flush(size_t key) {
    // BatchedEventsList res;
    // mBatcher.FlushQueue(key, res);
    // SerializeAndPush(std::move(res));
}
void FlusherArmsMetrics::FlushAll() {
    vector<BatchedEventsList> res;
    mBatcher.FlushAll(res);
    SerializeAndPush(std::move(res));
}


void FlusherArmsMetrics::SerializeAndPush(BatchedEventsList&& groupList) {
}


void FlusherArmsMetrics::SerializeAndPush(vector<BatchedEventsList>&& groupLists) {
    LOG_INFO(sLogger, ("[Metrics][SerializeAndPush] std::vector<BatchedEventsList> size", groupLists.size()));
    string serializeArmsMetricData, compressedData, serializeErrMsg;
    mGroupListSerializer->Serialize(std::move(groupLists), serializeArmsMetricData, serializeErrMsg);
    size_t packageSize = 0;
    packageSize += serializeArmsMetricData.size();
    if (mCompressor) {
        if (!mCompressor->Compress(serializeArmsMetricData, compressedData, serializeErrMsg)) {
            LOG_WARNING(mContext->GetLogger(),
                        ("failed to compress arms metrics event group", serializeErrMsg)("action", "discard data")(
                            "plugin", sName)("config", mContext->GetConfigName()));
            mContext->GetAlarm().SendAlarm(COMPRESS_FAIL_ALARM,
                                           "failed to compress arms event group: " + serializeErrMsg
                                               + "\taction: discard data\tplugin: " + sName
                                               + "\tconfig: " + mContext->GetConfigName(),
                                           mContext->GetProjectName(),
                                           mContext->GetLogstoreName(),
                                           mContext->GetRegion());
            return;
        }
    } else {
        compressedData = serializeArmsMetricData;
    }

    PushToQueue(std::move(compressedData), packageSize, RawDataType::EVENT_GROUP_LIST);
}

void FlusherArmsMetrics::PushToQueue(string&& data, size_t rawSize, RawDataType type) {
    SenderQueueItem* item = new SenderQueueItem(std::move(data), rawSize, this, mContext->GetProcessQueueKey());
    Sender::Instance()->PutIntoBatchMap(item, mRegion);
}

sdk::AsynRequest* FlusherArmsMetrics::BuildRequest(SenderQueueItem* item) const {
    map<string, string> httpHeader;
    httpHeader[logtail::sdk::CONTENT_TYPE] = "text/plain";
    httpHeader["content.encoding"] = "snappy";
    string body = item->mData;
    string HTTP_POST = "POST";
    auto host = GetArmsPrometheusGatewayHost();
    int32_t port = 80;
    auto operation = GetArmsPrometheusGatewayOperation();

    string queryString = "";
    int32_t mTimeout = 600;
    sdk::Response* response = new sdk::PostLogStoreLogsResponse();
    SendClosure* sendClosure = new SendClosure;
    sendClosure->mDataPtr = item;
    string mInterface = "";

    return new sdk::AsynRequest(HTTP_POST,
                                host,
                                port,
                                operation,
                                queryString,
                                httpHeader,
                                body,
                                mTimeout,
                                mInterface,
                                false,
                                sendClosure,
                                response);
}

std::string FlusherArmsMetrics::GetArmsPrometheusGatewayHost() const {
    std::string inner = "-intranet";
    std::string urlCommon = ".arms.aliyuncs.com";
    std::string host = mRegion + urlCommon;
    return host;
}

std::string FlusherArmsMetrics::GetArmsPrometheusGatewayOperation() const {
    std::string operation = "/collector/arms/ebpf/";
    operation.append(licenseKey);
    operation.append("/");
    operation.append(pushAppId);
    return operation;
}


} // namespace logtail
