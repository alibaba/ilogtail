#pragma once

#include "rw/IO.h"
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
        virtual void getRow(const int32_t rowIndex, std::vector<KV>& pairs, std::string& err);
        // get non-const columns
        //virtual void getColumn(const int32_t colIndex, std::vector<SplStringPiece>& values, std::string& err);
        virtual bool isColumnar(); 

    private:
        std::vector<std::string> mColumnNames;

        std::vector<std::string> mTmpSave;

        std::string timestamp =  "timestamp"; 
        std::string timestampNanosecond =  "timestampNanosecond"; 
        const PipelineEventGroup* mLogGroup;
};

using PipelineEventGroupInputPtr = std::shared_ptr<PipelineEventGroupInput>;

}  // namespace apsara::sls::spl