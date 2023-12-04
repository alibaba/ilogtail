#pragma once

#include "spl/rw/IO.h"
#include "models/PipelineEventGroup.h"
#include "models/LogEvent.h"

using namespace logtail;

namespace apsara::sls::spl {

class PipelineEventGroupOutput : public Output {
     public:
        PipelineEventGroupOutput(PipelineEventGroup& logGroup, std::vector<PipelineEventGroup>& logGroups, const std::string& taskLabel = "", bool withSleep = false)
            :mLogGroup(&logGroup), mLogGroupList(&logGroups), mTaskLabel(taskLabel), mWithSleep(withSleep) {}
        virtual ~PipelineEventGroupOutput() {
            
        }
        virtual void setHeader(const IOHeader& header, std::string& err);
        virtual void addRow(
            const std::vector<SplStringPiece>& row, 
            const uint32_t time,
            const uint32_t timeNsPart,
            const ErrorKV& errorKV, 
            std::string& error);
        virtual bool isColumnar();

    private:
        int32_t mRowCount;

        const IOHeader* mIOHeader;

        PipelineEventGroup* mLogGroup = nullptr;
        std::vector<PipelineEventGroup>* mLogGroupList;

        std::string mTaskLabel;
        bool mWithSleep;

        int32_t mTimeIdx = -1;
        int32_t mTimeNSIdx = -1;
        std::vector<int32_t> mTagsIdxs;
        std::vector<int32_t> mContentsIdxs;
        size_t lastTagStrHash = -1;
};

using PipelineEventGroupOutputPtr = std::shared_ptr<PipelineEventGroupOutput>;

}  // namespace apsara::sls::spl