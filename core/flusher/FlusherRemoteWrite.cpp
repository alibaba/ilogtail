#include "FlusherRemoteWrite.h"

#include "compression/CompressorFactory.h"
#include "sdk/Common.h"
#include "sender/Sender.h"

using namespace std;

namespace logtail {

const string FlusherRemoteWrite::sName = "flusher_remote_write";

FlusherRemoteWrite::FlusherRemoteWrite() {
    LOG_INFO(sLogger, ("LOG_INFO flusher", ""));
}


bool FlusherRemoteWrite::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    string errorMsg;
    LOG_INFO(sLogger, ("LOG_INFO flusher config", config.toStyledString()));

    // config
    // {
    //   "Type": "flusher_remote_write/flusher_push_gateway",
    //   "Endpoint": "cn-hangzhou.arms.aliyuncs.com",
    //   "Scheme": "https",
    //   "UserId": "******",
    //   "ClusterId": "*******",
    //   "Region": "cn-hangzhou"
    // }
    if (!config.isMember("Endpoint") || !config["Endpoint"].isString() || !config.isMember("Scheme")
        || !config["Scheme"].isString() || !config.isMember("UserId") || !config["UserId"].isString()
        || !config.isMember("ClusterId") || !config["ClusterId"].isString() || !config.isMember("Region")
        || !config["Region"].isString()) {
        errorMsg = "flusher_remote_write config error, must have Endpoint, Scheme, UserId, ClusterId, Region";
        LOG_ERROR(sLogger, ("LOG_ERROR flusher config", config.toStyledString())("error msg", errorMsg));
        return false;
    }
    mEndpoint = config["Endpoint"].asString();
    mScheme = config["Scheme"].asString();
    mUserId = config["UserId"].asString();
    mClusterId = config["ClusterId"].asString();
    mRegion = config["Region"].asString();

    mLogstoreKey = GenerateLogstoreFeedBackKey("remote_write_project", "remote_write_log_store");
    mSenderQueue = Sender::Instance()->GetSenderQueue(mLogstoreKey);

    mRemoteWritePath = "/prometheus/" + mUserId + "/" + mClusterId + "/" + mRegion + "/api/v2/write";

    // compressor
    mComperssor = CompressorFactory::GetInstance()->Create(config, *mContext, sName, CompressType::SNAPPY);
    DefaultFlushStrategyOptions strategy{
        static_cast<uint32_t>(1024), static_cast<uint32_t>(10), static_cast<uint32_t>(1)};
    if (!mBatcher.Init(Json::Value(), this, strategy, false)) {
        LOG_WARNING(sLogger, ("mBatcher init info:", "init err"));
        return false;
    }

    mGroupSerializer = make_unique<RemoteWriteEventGroupSerializer>(this);

    LOG_INFO(sLogger, ("init info:", "prometheus remote write init successful !"));

    return true;
}

void FlusherRemoteWrite::Send(PipelineEventGroup&& g) {
    LOG_INFO(sLogger, ("LOG_INFO flusher Send", sName));
    vector<BatchedEventsList> res;
    mBatcher.Add(std::move(g), res);
    SerializeAndPush(std::move(res));
}

void FlusherRemoteWrite::Flush(size_t key) {
    LOG_INFO(sLogger, ("LOG_INFO flusher Flush", sName));
    BatchedEventsList res;
    mBatcher.FlushQueue(key, res);
    SerializeAndPush(std::move(res));
}

void FlusherRemoteWrite::FlushAll() {
    LOG_INFO(sLogger, ("LOG_INFO flusher FlushAll", sName));
    vector<BatchedEventsList> res;
    mBatcher.FlushAll(res);
    SerializeAndPush(std::move(res));
}

sdk::AsynRequest* FlusherRemoteWrite::BuildRequest(SenderQueueItem* item) const {
    LOG_INFO(sLogger, ("LOG_INFO flusher BuildRequest", sName));
    RemoteWriteClosure* closure = new RemoteWriteClosure();
    sdk::Response* response = new sdk::Response();
    string httpMethod = "POST";
    bool httpsFlag = mScheme == "https";
    int32_t port = httpsFlag ? 443 : 80;
    map<string, string> headers;
    headers.insert(make_pair("Content-Encoding", "snappy"));
    headers.insert(make_pair("Content-Type", "application/x-protobuf"));
    headers.insert(make_pair("User-Agent", "RemoteWrite-v0.0.1"));
    headers.insert(make_pair("X-Prometheus-Remote-Write-Version", "0.1.0"));
    return new sdk::AsynRequest(
        httpMethod, mEndpoint, port, mRemoteWritePath, "", headers, item->mData, 30, "", httpsFlag, closure, response);
}

void FlusherRemoteWrite::SerializeAndPush(std::vector<BatchedEventsList>&& groupLists) {
    LOG_INFO(sLogger, ("LOG_INFO flusher SerializeAndPush", sName));
    for (auto& groupList : groupLists) {
        SerializeAndPush(std::move(groupList));
    }
}

void FlusherRemoteWrite::SerializeAndPush(BatchedEventsList&& groupList) {
    LOG_INFO(sLogger, ("LOG_INFO flusher SerializeAndPush", sName));
    for (auto& batchedEv : groupList) {
        string data, compressedData, errMsg;
        mGroupSerializer->Serialize(std::move(batchedEv), data, errMsg);
        if (!errMsg.empty()) {
            LOG_ERROR(sLogger, ("LOG_ERROR serialize event group", errMsg));
            continue;
        }

        size_t packageSize = 0;
        packageSize += data.size();
        if (mComperssor) {
            if (!mComperssor->Compress(data, compressedData, errMsg)) {
                LOG_WARNING(mContext->GetLogger(),
                            ("failed to compress arms metrics event group",
                             errMsg)("action", "discard data")("plugin", sName)("config", mContext->GetConfigName()));
                return;
            }
        } else {
            compressedData = data;
        }
        PushToQueue(std::move(compressedData), packageSize, RawDataType::EVENT_GROUP);
    }
}

void FlusherRemoteWrite::PushToQueue(string&& data, size_t rawSize, RawDataType type) {
    // TODO: mQueueKey && groupStrategy
    LOG_INFO(sLogger, ("LOG_INFO flusher PushToQueue", sName));
    SenderQueueItem* item = new SenderQueueItem(std::move(data), rawSize, this, mContext->GetProcessQueueKey(), type);
#ifdef APSARA_UNIT_TEST_MAIN
    mItems.push_back(item);
#else
    Sender::Instance()->PutIntoBatchMap(item, mRegion);
#endif
}

void RemoteWriteClosure::Done() {
    LOG_INFO(sLogger, ("remote write closure", "done"));
}

void RemoteWriteClosure::OnSuccess(sdk::Response* response) {
    LOG_INFO(sLogger, ("remote write closure", "success"));
    mPromise.set_value(RemoteWriteResponseInfo{response->statusCode, "", ""});
}

void RemoteWriteClosure::OnFail(sdk::Response* response,
                                const std::string& errorCode,
                                const std::string& errorMessage) {
    LOG_INFO(sLogger, ("remote write closure fail", errorCode + errorMessage));
    mPromise.set_value(RemoteWriteResponseInfo{response->statusCode, errorCode, errorMessage});
}

} // namespace logtail
