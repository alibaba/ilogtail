//
// Created by lurious on 2024/6/17.
//

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "arms_metrics_pb/MeasureBatches.pb.h"
#include "serializer/Serializer.h"
#include "common/MachineInfoUtil.h"

namespace logtail {

class ArmsMetricsEventGroupListSerializer : public Serializer<std::vector<BatchedEventsList>> {
public:
    ArmsMetricsEventGroupListSerializer(Flusher* f) : Serializer<std::vector<BatchedEventsList>>(f) {}

    bool Serialize(std::vector<BatchedEventsList>&& v, std::string& res, std::string& errorMsg) override;

private:
    void ConvertBatchedEventsListToMeasureBatch(BatchedEventsList&& batchedEventsList,
                                                proto::MeasureBatches* measureBatches);
    void ConvertBatchedEventsToMeasures(BatchedEvents&& batchedEvents, proto::MeasureBatch* measureBatch);
    void ConvertEventsToMeasure(EventsContainer&& events, proto::Measures* measures);
    int64_t GetMeasureTimestamp(BatchedEvents& batchedEvents);

    proto::EnumUnit GetUnitByMetricName(std::string metricName);
    std::string GetValueTypeByMetricName(std::string metricName);
    std::string GetIpFromTags(SizedMap& mTags);
    std::string GetAppIdFromTags(SizedMap& mTags);
};

class ArmsSpanEventGroupListSerializer : public Serializer<std::vector<BatchedEventsList>> {
public:
    ArmsSpanEventGroupListSerializer(Flusher* f) : Serializer<std::vector<BatchedEventsList>>(f) {
        common_resources_ = {
            {"arms.regionId", "cn-beijing"},
            {"service.name", "mall-user-server"},
            {"host.name", GetHostName()},
            {"host.ip", GetHostIp()},
            {"app.type", "ebpf"},
            {"cluster.id", "@qianlu.kk TODO"},
            {"telemetry.sdk.name", "oneagent"},
            {"telemetry.sdk.version", "ebpf"},
        };
    }

    bool Serialize(std::vector<BatchedEventsList>&& v, std::string& res, std::string& errorMsg) override;

private:
    std::map<std::string, std::string> common_resources_;
};

} // namespace logtail