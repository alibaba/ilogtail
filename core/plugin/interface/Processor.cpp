#include "plugin/interface/Processor.h"

namespace logtail {

    void Processor::Process(std::vector<PipelineEventGroup>& logGroupList) {
        Process(logGroupList[0]);
    }
}
