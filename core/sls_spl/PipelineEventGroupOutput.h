#pragma once

#include "rw/IO.h"
#include "models/PipelineEventGroup.h"
#include "models/LogEvent.h"

using namespace logtail;

namespace apsara::sls::spl {

class PipelineEventGroupOutput : public Output {
   private:
    std::string taskLabel_;
    int32_t rowSize_;
    int32_t currentRow_;
    std::vector<SplStringPiece> columnNames_;
    std::unordered_map<int32_t, std::string> constColumns_;
    bool withSleep_;

    PipelineEventGroup* logGroups_;
    EventsContainer* newEvents_;


   public:
    PipelineEventGroupOutput(PipelineEventGroup& logGroups, EventsContainer& newEvents, const std::string& taskLabel = "", bool withSleep = false)
        :logGroups_(&logGroups), newEvents_(&newEvents), taskLabel_(taskLabel), withSleep_(withSleep) {}
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