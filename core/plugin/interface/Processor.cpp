#include "plugin/interface/Processor.h"

namespace logtail {

    void Processor::Process(std::vector<PipelineEventGroup>& logGroupList) {
        if (logGroupList.size() <= 0) {
            return;
        } 
        Process(logGroupList[0]);
    }
}
