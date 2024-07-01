//
// Created by lurious on 2024/6/17.
//
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "batch/BatchStatus.h"
#include "batch/Batcher.h"
#include "common/LogstoreFeedbackKey.h"
#include "compression/Compressor.h"
#include "json/json.h"
#include "models/PipelineEventGroup.h"
#include "plugin/interface/Flusher.h"
#include "serializer/ArmsSerializer.h"


namespace logtail {

class FlusherArmsMetrics : public Flusher {
public:
    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config, Json::Value& optionalGoPipeline) override;
    bool Register() override;
    bool Unregister(bool isPipelineRemoving) override;
    void Send(PipelineEventGroup&& g) override;
    void Flush(size_t key) override;
    void FlushAll() override;
    sdk::AsynRequest* BuildRequest(SenderQueueItem* item) const override;

    static const std::string sName;
    FlusherArmsMetrics();

    // SingleLogstoreSenderManager<SenderQueueParam>* GetSenderQueue() const { return mSenderQueue; }

    std::string mProject;
    std::string mRegion;
    std::string mAliuid;
    std::string licenseKey;
    std::string pushAppId;
    std::unique_ptr<Compressor> mCompressor;


private:
    void SerializeAndPush(std::vector<BatchedEventsList>&& groupLists);
    void SerializeAndPush(BatchedEventsList&& groupList);

    std::string GetArmsPrometheusGatewayHost() const;
    std::string GetArmsPrometheusGatewayOperation() const;

    Batcher<SLSEventBatchStatus> mBatcher;
    std::unique_ptr<Serializer<std::vector<BatchedEventsList>>> mGroupListSerializer;

    std::unique_ptr<PipelineEventGroup> mEventGroup;

    void PushToQueue(std::string&& data, size_t rawSize, RawDataType type);

    BatchedEventsList ConvertToBatchedList(PipelineEventGroup&& g);
};


} // namespace logtail
