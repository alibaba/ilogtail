#pragma once

#include <variant>
#include <atomic>
#include <map>
#include <array>
#include <memory>

#include "pipeline/PipelineContext.h"
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
                        const std::variant<SecurityOptions*, nami::ObserverProcessOption*, nami::ObserverFileOption*, nami::ObserverNetworkOption*> options);

    bool DisablePlugin(const std::string& pipeline_name, nami::PluginType type);

    bool SuspendPlugin(const std::string& pipeline_name, nami::PluginType type);

private:
    bool StartPluginInternal(const std::string& pipeline_name, uint32_t plugin_index,
                        nami::PluginType type, 
                        const logtail::PipelineContext* ctx, 
                        const std::variant<SecurityOptions*, nami::ObserverProcessOption*, nami::ObserverFileOption*, nami::ObserverNetworkOption*> options);
    eBPFServer() = default;
    ~eBPFServer() = default;

    std::unique_ptr<SourceManager> mSourceManager;
    // source manager
    std::unique_ptr<MeterHandler> mMeterCB;
    std::unique_ptr<SpanHandler> mSpanCB;
    std::unique_ptr<SecurityHandler> mNetworkSecureCB;
    std::unique_ptr<SecurityHandler> mProcessSecureCB;
    std::unique_ptr<SecurityHandler> mFileSecureCB;

    eBPFAdminConfig mAdminConfig;
    std::atomic_bool mInited;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class eBPFServerUnittest;
#endif
};

} // namespace ebpf
} // namespace logtail