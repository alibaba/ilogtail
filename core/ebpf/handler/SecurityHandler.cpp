#include "ebpf/handler/SecurityHandler.h"
#include "logger/Logger.h"
#include "pipeline/PipelineContext.h"
#include "common/RuntimeUtil.h"
#include "ebpf/SourceManager.h"
#include "models/SpanEvent.h"
#include "models/PipelineEventGroup.h"
#include "models/PipelineEvent.h"
#include "logger/Logger.h"
#include "queue/ProcessQueueManager.h"
#include "queue/ProcessQueueItem.h"
#include "common/MachineInfoUtil.h"

namespace logtail {
namespace ebpf {

SecurityHandler::SecurityHandler(logtail::PipelineContext* ctx, uint32_t idx) 
    : AbstractHandler(ctx, idx) {
    mHostName = GetHostName();
    mHostIp = GetHostIp();
}

void SecurityHandler::handle(std::vector<std::unique_ptr<AbstractSecurityEvent>>&& events) {
    if (mCtx == nullptr || events.empty()) {
        return ;
    }
    std::shared_ptr<SourceBuffer> source_buffer = std::make_shared<SourceBuffer>();;
    PipelineEventGroup event_group(source_buffer);
    // aggregate to pipeline event group
    // set host ips
    event_group.SetTag("host.ip", mHostIp);
    event_group.SetTag("host.name", mHostName);
    for (auto& x : events) {
        auto event = event_group.AddLogEvent();
        for (auto& tag : x->GetAllTags()) {
            event->SetContent(tag.first, tag.second);
        }
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::nanoseconds(x->GetTimestamp()));
        event->SetTimestamp(seconds.count(), x->GetTimestamp());
    }
    #ifdef APSARA_UNIT_TEST_MAIN
        mProcessTotalCnt+= events.size();
    #endif
    std::unique_ptr<ProcessQueueItem> item = 
            std::unique_ptr<ProcessQueueItem>(new ProcessQueueItem(std::move(event_group), mPluginIdx));
    
    if (ProcessQueueManager::GetInstance()->PushQueue(mCtx->GetProcessQueueKey(), std::move(item))) {
        LOG_WARNING(sLogger, ("Push queue failed!", events.size()));
    } else {
        LOG_INFO(sLogger, ("Push queue success!", events.size()));
    }
}

}
}