#pragma once

#include "rw/IO.h"
#include "models/PipelineEventGroup.h"
#include "models/LogEvent.h"

using namespace logtail;

namespace apsara::sls::spl {

class PipelineEventGroupOutput : public Output {
   private:
    std::string mTaskLabel;
    int32_t mRowSize;
    std::vector<SplStringPiece> mColumnNames;
    std::unordered_map<int32_t, std::string> mConstColumns;
    bool mWithSleep;

    PipelineEventGroup* mLogGroup;
    EventsContainer* mNewEvents;


   public:
    PipelineEventGroupOutput(PipelineEventGroup& logGroups, EventsContainer& newEvents, const std::string& taskLabel = "", bool withSleep = false)
        :mLogGroup(&logGroups), mNewEvents(&newEvents), mTaskLabel(taskLabel), mWithSleep(withSleep) {}
    virtual ~PipelineEventGroupOutput() {
        
    }
    virtual void setHeader(const IOHeader& header, std::string& err);
    virtual void addRow(
        const std::vector<SplStringPiece>& row, 
        const ErrorKV& errorKV, 
        std::string& error);
    virtual void finish(std::string& error);
};

using PipelineEventGroupOutputPtr = std::shared_ptr<PipelineEventGroupOutput>;

}  // namespace apsara::sls::spl