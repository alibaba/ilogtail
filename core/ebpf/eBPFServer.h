#pragma once

#include <variant>
#include <atomic>
#include <map>
#include <array>

#include "pipeline/PipelineContext.h"
#include "ebpf/observer/ObserverOptions.h"
#include "ebpf/security/SecurityOptions.h"
#include "ebpf/SourceManager.h"
#include "ebpf/config.h"
#include "ebpf/include/export.h"
#include "ebpf/handler/AbstractHandler.h"
#include "ebpf/handler/ObserveHandler.h"
#include "ebpf/handler/SecurityHandler.h"


namespace logtail {
namespace ebpf {

class eBPFServer {
public:
    eBPFServer(const eBPFServer&) = delete;
    eBPFServer& operator=(const eBPFServer&) = delete;

    void Init();

    static eBPFServer* GetInstance() {
        static eBPFServer instance;
        return &instance;
    }

    void Stop();

    bool EnablePlugin(const std::string& pipeline_name, uint32_t plugin_index,
                        nami::PluginType type, 
                        const logtail::PipelineContext* ctx, 
                        const std::variant<SecurityOptions*, ObserverOptions*> options);
    // bool UpdatePlugin(const std::string& pipeline_name, uint32_t plugin_index,
    //                     nami::PluginType type, 
    //                     const logtail::PipelineContext* ctx, 
    //                     const std::variant<SecurityOptions*, ObserverOptions*> options);
    bool DisablePlugin(const std::string& pipeline_name, nami::PluginType type);
    bool SuspendPlugin(const std::string& pipeline_name, nami::PluginType type);

private:
    bool StartPluginInternal(const std::string& pipeline_name, uint32_t plugin_index,
                        nami::PluginType type, 
                        const logtail::PipelineContext* ctx, 
                        const std::variant<SecurityOptions*, ObserverOptions*> options);
    eBPFServer() = default;
    ~eBPFServer() = default;

    // source manager
    std::unique_ptr<MeterHandler> ob_meter_cb_;
    std::unique_ptr<SpanHandler> ob_span_cb_;
    std::unique_ptr<SecurityHandler> network_secure_cb_;
    std::unique_ptr<SecurityHandler> process_secure_cb_;
    std::unique_ptr<SecurityHandler> file_secure_cb_;

    const int32_t defaultReceiveEventChanCap = 4096;
    const bool defaultAdminDebugMode = false;
    const std::string defaultAdminLogLevel = "warn";
    const bool defaultAdminPushAllSpan = false;
    const int32_t defaultAggregationAggWindowSecond = 15;
    const std::string defaultConverageStrategy = "combine";
    const std::string defaultSampleStrategy = "fixedRate";
    const double defaultSampleRate = 0.01;
    const int32_t defaultSocketSlowRequestThresholdMs = 500;
    const int32_t defaultSocketMaxConnTrackers = 10000;
    const int32_t defaultSocketMaxBandWidthMbPerSec = 30;
    const int32_t defaultSocketMaxRawRecordPerSec = 100000;
    const int32_t defaultProfileSampleRate = 10;
    const int32_t defaultProfileUploadDuration = 10;
    const bool defaultProcessEnableOOMDetect = false;
    eBPFConfig config_;
    std::atomic_bool inited_;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class eBPFServerUnittest;
#endif
};

} // namespace ebpf
} // namespace logtail