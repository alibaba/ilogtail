#include "plugin/interface/Processor.h"

namespace logtail {

    void Processor::Process(std::vector<PipelineEventGroup>& logGroupList) {
        for (auto& logGroup : logGroupList) {
            Process(logGroup);
        }
    }
}
