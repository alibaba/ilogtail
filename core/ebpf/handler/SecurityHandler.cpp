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
    host_name_ = GetHostName();
    host_ip_ = GetHostIp();
}

void SecurityHandler::handle(std::vector<std::unique_ptr<AbstractSecurityEvent>>&& events) {
    if (ctx_ == nullptr || events.empty()) {
        return ;
    }
    std::shared_ptr<SourceBuffer> source_buffer = std::make_shared<SourceBuffer>();;
    PipelineEventGroup mTestEventGroup(source_buffer);
    // aggregate to pipeline event group
    // set host ips
    mTestEventGroup.SetTag("host_ip", host_ip_);
    mTestEventGroup.SetTag("host_name", host_name_);
    for (auto& x : events) {
        auto event = mTestEventGroup.AddLogEvent();
        for (auto& tag : x->GetAllTags()) {
            event->SetContent(tag.first, tag.second);
        }

        time_t ts = static_cast<time_t>(x->GetTimestamp() / 1e9);
        event->SetTimestamp(ts);
    }
    #ifdef APSARA_UNIT_TEST_MAIN
        process_total_count_+= events.size();
    #endif
    std::unique_ptr<ProcessQueueItem> item = 
            std::unique_ptr<ProcessQueueItem>(new ProcessQueueItem(std::move(mTestEventGroup), plugin_idx_));
    
    if (ProcessQueueManager::GetInstance()->PushQueue(ctx_->GetProcessQueueKey(), std::move(item))) {
        LOG_WARNING(sLogger, ("Push queue failed!", events.size()));
    } else {
        LOG_INFO(sLogger, ("Push queue success!", events.size()));
    }
}

}
}