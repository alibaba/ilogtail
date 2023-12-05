#pragma once

#include "spl/rw/IO.h"
#include "models/PipelineEventGroup.h"
#include "models/LogEvent.h"

using namespace logtail;

namespace apsara::sls::spl {

class PipelineEventGroupInput : public Input {
    public:
        PipelineEventGroupInput(const std::vector<std::string> columnNames, const PipelineEventGroup& logGroup)
            : mColumnNames(columnNames), mLogGroup(&logGroup) {}

        ~PipelineEventGroupInput() {}

        virtual void getHeader(IOHeader& header, std::string& err);
        virtual void getColumn(const int32_t colIndex, std::vector<SplStringPiece>& values, std::string& err);
        virtual void getTimeColumns(std::vector<uint32_t>& times,std::vector<uint32_t>& timeNanos, std::string& err);
        virtual bool isColumnar(); 
        virtual bool hasTime(); 


    private:
        std::vector<std::string> mColumnNames;

        std::vector<std::string> mTmpTags;
        const PipelineEventGroup* mLogGroup;
};

using PipelineEventGroupInputPtr = std::shared_ptr<PipelineEventGroupInput>;

}  // namespace apsara::sls::spl