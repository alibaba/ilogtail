#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include <map>

#include "serializer/Serializer.h"
#include "batch/BatchStatus.h"
#include "batch/Batcher.h"
#include "common/LogstoreFeedbackKey.h"
#include "compression/Compressor.h"
#include "json/json.h"
#include "models/PipelineEventGroup.h"
#include "plugin/interface/Flusher.h"
#include "serializer/ArmsSerializer.h"

namespace logtail {

class FlusherXTraceSpan : public Flusher {
public:
    FlusherXTraceSpan();

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config, Json::Value& optionalGoPipeline) override;
    bool Register() override;
    bool Unregister(bool isPipelineRemoving) override;
    void Send(PipelineEventGroup&& g) override;
    void Flush(size_t key) override;
    void FlushAll() override;
    sdk::AsynRequest* BuildRequest(SenderQueueItem* item) const override;
    std::map<std::string, std::string> GetCommonResources();

    static const std::string sName;

private:
    void SerializeAndPush(BatchedEventsList&& groupList);
    void SerializeAndPush(std::vector<BatchedEventsList>&& groupLists);

    std::string GetXtraceEndpoint() const;
    std::string GetXtraceUrl() const;

    Batcher<SLSEventBatchStatus> batcher_;
    std::string region_;
    std::string user_id_;
    std::string app_id_;
    std::string license_key_;
    std::map<std::string, std::string> endpoints_;
    // std::string app_id_;

    std::unique_ptr<Compressor> compressor_;
    ArmsSpanEventGroupListSerializer serializer_;
    std::map<std::string, std::string> common_resources_;
};

}