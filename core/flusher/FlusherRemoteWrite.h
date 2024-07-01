#pragma once

#include "batch/Batcher.h"
#include "compression/Compressor.h"
#include "plugin/interface/Flusher.h"
#include "sdk/Closure.h"
#include "serializer/RemoteWriteSerializer.h"

namespace logtail {

struct RemoteWriteResponseInfo {
    int32_t statusCode;
    std::string errorCode;
    std::string errorMessage;
};

class RemoteWriteClosure : public sdk::LogsClosure {
public:
    void Done() override {}
    void OnSuccess(sdk::Response* response) override;
    void OnFail(sdk::Response* response, const std::string& errorCode, const std::string& errorMessage) override;
    std::promise<RemoteWriteResponseInfo> mPromise;
};

class FlusherRemoteWrite : public Flusher {
public:
    static const std::string sName;

    FlusherRemoteWrite();

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config, Json::Value& optionalGoPipeline) override;
    void Send(PipelineEventGroup&& g) override;
    void Flush(size_t key) override;
    void FlushAll() override;
    sdk::AsynRequest* BuildRequest(SenderQueueItem* item) const override;


private:
    void SerializeAndPush(std::vector<BatchedEventsList>&& groupLists);
    void SerializeAndPush(BatchedEventsList&& groupList);

    Batcher<> mBatcher;
    std::unique_ptr<EventGroupSerializer> mGroupSerializer;
    std::unique_ptr<Compressor> mComperssor;

    std::string mEndpoint;
    std::string mRemoteWritePath;
    std::string mScheme;
    std::string mUserId;
    std::string mClusterId;
    std::string mRegion;


#ifdef APSARA_UNIT_TEST_MAIN
    std::vector<SenderQueueItem*> mItems;
    friend class FlusherRemoteWriteTest;
#endif
};

} // namespace logtail
