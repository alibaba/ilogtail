/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <json/json.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "batch/BatchStatus.h"
#include "batch/Batcher.h"
#include "compression/Compressor.h"
#include "models/PipelineEventGroup.h"
#include "plugin/interface/HttpFlusher.h"
#include "queue/SenderQueueItem.h"
#include "sender/ConcurrencyLimiter.h"
#include "serializer/SLSSerializer.h"

namespace logtail {

class FlusherSLS : public HttpFlusher {
    // TODO: temporarily used
    friend class ProfileSender;

public:
    enum class TelemetryType { LOG, METRIC };

    static std::shared_ptr<ConcurrencyLimiter> GetProjectConcurrencyLimiter(const std::string& project);
    static std::shared_ptr<ConcurrencyLimiter> GetRegionConcurrencyLimiter(const std::string& region);
    static void ClearInvalidConcurrencyLimiters();

    static void RecycleResourceIfNotUsed();

    static std::string GetDefaultRegion();
    static void SetDefaultRegion(const std::string& region);
    static std::string GetAllProjects();
    static bool IsRegionContainingConfig(const std::string& region);

    // TODO: should be moved to enterprise config provider
    static bool GetRegionStatus(const std::string& region);
    static void UpdateRegionStatus(const std::string& region, bool status);

    static const std::string sName;

    FlusherSLS();

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config, Json::Value& optionalGoPipeline) override;
    bool Start() override;
    bool Stop(bool isPipelineRemoving) override;
    bool Send(PipelineEventGroup&& g) override;
    bool Flush(size_t key) override;
    bool FlushAll() override;
    std::unique_ptr<AsynHttpRequest<SenderQueueItem>> BuildRequest(SenderQueueItem* item) const override;
    void OnSendDone(const HttpResponse& response, SenderQueueItem* item) override;

    CompressType GetCompressType() const { return mCompressor ? mCompressor->GetCompressType() : CompressType::NONE; }

    // for use of Go pipeline, stream, observer and shennong
    bool Send(std::string&& data, const std::string& shardHashKey, const std::string& logstore = "");

    std::string mProject;
    std::string mLogstore;
    std::string mRegion;
    std::string mEndpoint;
    std::string mAliuid;
    TelemetryType mTelemetryType = TelemetryType::LOG;
    std::vector<std::string> mShardHashKeys;
    uint32_t mMaxSendRate = 0; // preserved only for exactly once
    uint32_t mFlowControlExpireTime = 0;

private:
    static const std::unordered_set<std::string> sNativeParam;

    static void InitResource();

    static void IncreaseProjectReferenceCnt(const std::string& project);
    static void DecreaseProjectReferenceCnt(const std::string& project);
    static void IncreaseRegionReferenceCnt(const std::string& region);
    static void DecreaseRegionReferenceCnt(const std::string& region);

    static std::mutex sMux;
    static std::unordered_map<std::string, std::weak_ptr<ConcurrencyLimiter>> sProjectConcurrencyLimiterMap;
    static std::unordered_map<std::string, std::weak_ptr<ConcurrencyLimiter>> sRegionConcurrencyLimiterMap;

    static std::mutex sDefaultRegionLock;
    static std::string sDefaultRegion;

    static std::mutex sProjectRefCntMapLock;
    static std::unordered_map<std::string, int32_t> sProjectRefCntMap;
    static std::mutex sRegionRefCntMapLock;
    static std::unordered_map<std::string, int32_t> sRegionRefCntMap;

    // TODO: should be moved to enterprise config provider
    static std::mutex sRegionStatusLock;
    static std::unordered_map<std::string, bool> sAllRegionStatus;

    void GenerateGoPlugin(const Json::Value& config, Json::Value& res) const;
    bool SerializeAndPush(std::vector<BatchedEventsList>&& groupLists);
    bool SerializeAndPush(BatchedEventsList&& groupList);
    bool SerializeAndPush(PipelineEventGroup&& g); // for exactly once only
    bool PushToQueue(QueueKey key, std::unique_ptr<SenderQueueItem>&& item, uint32_t retryTimes = 500);
    std::string GetShardHashKey(const BatchedEvents& g) const;
    void AddPackId(BatchedEvents& g) const;

    Batcher<SLSEventBatchStatus> mBatcher;
    std::unique_ptr<EventGroupSerializer> mGroupSerializer;
    std::unique_ptr<Serializer<std::vector<CompressedLogGroup>>> mGroupListSerializer;
    std::unique_ptr<Compressor> mCompressor;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class FlusherSLSUnittest;
#endif
};

sls_logs::SlsCompressType ConvertCompressType(CompressType type);

} // namespace logtail
