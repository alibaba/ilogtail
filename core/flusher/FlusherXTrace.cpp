#include "flusher/FlusherXTrace.h"
#include "sender/Sender.h"
#include "batch/FlushStrategy.h"
#include "compression/CompressorFactory.h"
#include "logger/Logger.h"

DEFINE_FLAG_INT32(span_batch_send_interval, "batch sender interval (second)(default 5)", 5);
DEFINE_FLAG_INT32(span_merge_count_limit, "span count in one spanGroup at most", 100);
DEFINE_FLAG_INT32(span_batch_send_size,
                  "batch send span size limit(bytes)(default 256KB)",
                  256 * 1024);

namespace logtail {

const std::string FlusherXTraceSpan::sName = "flusher_xtrace_span";

FlusherXTraceSpan::FlusherXTraceSpan()
        : serializer_(ArmsSpanEventGroupListSerializer(this)) {
    endpoints_ = {
{	"cn-huhehaote",          "dc-cn-huhehaote-internal.arms.aliyuncs.com",
},{	"arms-dc-me-central-1",  "arms-dc-me-central-1-internal.aliyuncs.com",
},{	"cn-beijing",            "arms-dc-bj-internal.aliyuncs.com",
},{	"ap-southeast-1-oxs",    "dc-ap-southeast-1-oxs-internal.arms.aliyuncs.com",
},{	"ap-southeast-2",        "dc-ap-southeast-2-internal.arms.aliyuncs.com",
},{	"cn-north-2-gov-1",      "arms-dc-gov-internal.aliyuncs.com",
},{	"cn-qingdao",            "arms-dc-qd-internal.aliyuncs.com",
},{	"cn-wulanchabu",         "dc-cn-wulanchabu-internal.arms.aliyuncs.com",
},{	"cn-zhangjiakou-oxs",    "arms-zb-oxs-internal.aliyuncs.com",
},{	"eu-west-1",             "dc-eu-west-1-internal.arms.aliyuncs.com",
},{	"ap-southeast-5",        "arms-dc-indonesia-internal.aliyuncs.com",
},{	"ap-northeast-1-oxs",    "dc-ap-northeast-1-oxs-internal.arms.aliyuncs.com",
},{	"cn-hangzhou-finance",   "arms-dc-hz-finance-internal.aliyuncs.com",
},{	"cn-shenzhen-finance-1", "arms-dc-sz-finance-internal.aliyuncs.com",
},{	"cn-shanghai-finance-1", "arms-dc-sh-finance-internal.aliyuncs.com",
},{	"eu-central-1",          "arms-dc-frankfurt-internal.aliyuncs.com",
},{	"cn-shanghai",           "arms-dc-sh-internal.aliyuncs.com",
},{	"cn-chengdu",            "dc-cn-chengdu-internal.arms.aliyuncs.com",
},{	"ap-southeast-3",        "dc-ap-southeast-3.arms.aliyuncs.com",
},{	"ap-northeast-1",        "arms-dc-jp-internal.aliyuncs.com",
},{	"us-west-1",             "arms-dc-usw-internal.aliyuncs.com",
},{	"cn-shenzhen",           "arms-dc-sz-internal.aliyuncs.com",
},{	"cn-hongkong",           "arms-dc-hk-internal.aliyuncs.com",
},{	"ap-southeast-6",        "arms-dc-ap-southeast-6-internal.aliyuncs.com",
},{	"dc-me-east-1",          "dc-me-east-1-internal.arms.aliyuncs.com",
},{	"ap-southeast-1",        "arms-dc-sg-internal.aliyuncs.com",
},{	"cn-guangzhou",          "dc-cn-guangzhou-internal.arms.aliyuncs.com",
},{	"cn-zhangjiakou",        "arms-dc-zb-internal.aliyuncs.com",
},{	"us-east-1",             "dc-us-east-1-internal.arms.aliyuncs.com",
},{	"cn-hangzhou",           "arms-dc-hz-internal.aliyuncs.com",
},{	"cn-heyuan",             "dc-cn-heyuan-internal.arms.aliyuncs.com",
},{	"us-east-1-oxs",         "dc-us-east-1-oxs-internal.arms.aliyuncs.com",
},{	"eu-central-1-oxs",      "dc-eu-central-1-oxs-internal.arms.aliyuncs.com",
},{	"ap-south-1",            "dc-ap-south-1-internal.arms.aliyuncs.com",
}
    };
}

bool FlusherXTraceSpan::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    // parse config
    region_ = "cn-hangzhou";
    std::string errorMsg;
    // Region
    if (!GetOptionalStringParam(config, "Region", region_, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              region_,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }
    // Licensekey
    if (!GetOptionalStringParam(config, "Licensekey", license_key_, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              region_,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }
    // UserId
    if (!GetOptionalStringParam(config, "UserId", user_id_, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              region_,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }
    // AppId
    if (!GetOptionalStringParam(config, "AppId", app_id_, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              region_,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }
    common_resources_ = {
            {"arms.appId", app_id_},
            {"arms.regionId", region_},
            {"service.name", "mall-user-server"},
            {"host.name", GetHostName()},
            {"host.ip", GetHostIp()},
            {"app.type", "ebpf"},
            {"cluster.id", "@qianlu.kk TODO"},
            {"telemetry.sdk.name", "oneagent"},
            {"telemetry.sdk.version", "ebpf"},
        };

    // init compressor
    compressor_ = CompressorFactory::GetInstance()->Create(config, *mContext, sName, CompressType::SNAPPY);

    DefaultFlushStrategyOptions strategy{static_cast<uint32_t>(INT32_FLAG(span_batch_send_interval)),
                                         static_cast<uint32_t>(INT32_FLAG(span_merge_count_limit)),
                                         static_cast<uint32_t>(INT32_FLAG(span_batch_send_size))};
    if (!batcher_.Init(Json::Value(), this, strategy, true)) {
        // when either exactly once is enabled or ShardHashKeys is not empty, we don't enable group batch
        LOG_WARNING(sLogger, ("[XTraceFlusher] batcher_ init info: ", "init err!"));
        return false;
    }
    LOG_INFO(sLogger, ("[XTraceFlusher] init info: ", "xtrace flusher init successful !")); 

    auto mLogstoreKey = GenerateLogstoreFeedBackKey("arms_span_proj", "arms_span_proj");
    mSenderQueue = Sender::Instance()->GetSenderQueue(mLogstoreKey);

    return true;
}


bool FlusherXTraceSpan::Register() {
    return true;
}
bool FlusherXTraceSpan::Unregister(bool isPipelineRemoving) {
    return true;
}

void FlusherXTraceSpan::Send(PipelineEventGroup&& g) {
    LOG_INFO(sLogger, ("[Send] ", " enter "));
    std::vector<BatchedEventsList> res;
    batcher_.Add(std::move(g), res);
    SerializeAndPush(std::move(res));
}
void FlusherXTraceSpan::Flush(size_t key) {
    LOG_INFO(sLogger, ("[Flush] ", " enter "));
    BatchedEventsList res;
    batcher_.FlushQueue(key, res);
    SerializeAndPush(std::move(res));
}
void FlusherXTraceSpan::FlushAll() {
    LOG_INFO(sLogger, ("[FlushAll] ", " enter "));
    std::vector<BatchedEventsList> res;
    batcher_.FlushAll(res);
    SerializeAndPush(std::move(res));
}

void FlusherXTraceSpan::SerializeAndPush(BatchedEventsList&& groupList) {
    LOG_INFO(sLogger, ("[SerializeAndPush] BatchedEventsList ", " enter??? "));
}

void FlusherXTraceSpan::SerializeAndPush(std::vector<BatchedEventsList>&& groupLists) {
    LOG_INFO(sLogger, ("[Span][SerializeAndPush] std::vector<BatchedEventsList> enter, size", groupLists.size()));
    LOG_INFO(sLogger, ("SerializeAndPush groupLists size :", groupLists.size()));
    std::string serializedData, compressedData, serializeErrMsg;
    serializer_.Serialize(std::move(groupLists), serializedData, serializeErrMsg);
    // LOG_INFO(sLogger, ("serialed res info:", serializeArmsMetricData));
    size_t packageSize = 0;
    packageSize += serializedData.size();
    if (compressor_) {
        if (!compressor_->Compress(serializedData, compressedData, serializeErrMsg)) {
            LOG_WARNING(mContext->GetLogger(),
                        ("failed to compress xtrace span event group", serializeErrMsg)("action", "discard data")(
                            "plugin", sName)("config", mContext->GetConfigName()));
            // mContext->GetAlarm().SendAlarm(COMPRESS_FAIL_ALARM,
            //                                "failed to compress arms event group: " + serializeErrMsg
            //                                    + "\taction: discard data\tplugin: " + sName
            //                                    + "\tconfig: " + mContext->GetConfigName(),
            //                                mContext->GetProjectName(),
            //                                mContext->GetLogstoreName(),
            //                                mContext->GetRegion());
            return;
        }
    } else {
        compressedData = serializedData;
    }
    LOG_INFO(sLogger, ("[XTraceFlusher] compressedData serialed size:", compressedData.size()));

    SenderQueueItem* item = new SenderQueueItem(std::move(compressedData),
                                                packageSize,
                                                this,
                                                mContext->GetProcessQueueKey());
    Sender::Instance()->PutIntoBatchMap(item);
    LOG_INFO(sLogger, ("[XTraceFlusher] successfully push into batch map", 1));
}

std::string FlusherXTraceSpan::GetXtraceEndpoint() const {
    std::string endpoint = "arms-dc-hz.aliyuncs.com";
    auto it = endpoints_.find(region_);
    if (it != endpoints_.end()) {
        endpoint = it->second;
    }
    return endpoint;
}

std::string FlusherXTraceSpan::GetXtraceUrl() const {
    std::string url = "/api/v1/arms/otel/" + license_key_ + "/" + app_id_;
    LOG_INFO(sLogger, ("[GetXtraceUrl] url", url));
    return url;
}

sdk::AsynRequest* FlusherXTraceSpan::BuildRequest(SenderQueueItem* item) const {
    std::map<std::string, std::string> httpHeader;
    httpHeader[logtail::sdk::CONTENT_TYPE] = "application/x-protobuf";
    // TODO @qianlu.kk 
    httpHeader["licenseKey"] = license_key_;
    httpHeader["content.type"] = "span";
    httpHeader["X-ARMS-Encoding"] = "snappy";
    httpHeader["data.type"] = "";
    std::string compressBody = item->mData;
    std::string HTTP_POST = "POST";
    auto host = GetXtraceEndpoint(); // arms-dc-bj-internal.aliyuncs.com
    std::string url = GetXtraceUrl(); // /api/v1/arms/otel/awy7aw18hz@2694ecf80a44b70/awy7aw18hz@72e78405b0019f2
    int32_t port = 80;
    // TODO @qianlu add query string
    std::string queryString = "";
    int32_t mTimeout = 600;
    // TODO @qianlu substitute to xtrace resp body
    sdk::Response* response = new sdk::PostLogStoreLogsResponse();
    SendClosure* sendClosure = new SendClosure;
    sendClosure->mDataPtr = item;
    std::string mInterface = "";

    return new sdk::AsynRequest(HTTP_POST,
                                host,
                                port,
                                url,
                                queryString,
                                httpHeader,
                                compressBody,
                                mTimeout,
                                mInterface,
                                false,
                                sendClosure,
                                response);
}

}