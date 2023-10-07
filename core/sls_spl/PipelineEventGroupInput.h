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
        virtual void getRow(const int32_t rowIndex, std::vector<KV>& pairs, uint32_t& time, uint32_t& timeNsPart, std::string& err);
        // get non-const columns
        virtual void getColumn(const int32_t colIndex, std::vector<SplStringPiece>& values, std::string& err);
        virtual bool isColumnar(); 


    private:
        std::vector<std::string> mColumnNames;

        // 存储时间戳转换后的字符串
        std::vector<std::string> mTmpSave;

        const PipelineEventGroup* mLogGroup;
};

using PipelineEventGroupInputPtr = std::shared_ptr<PipelineEventGroupInput>;

}  // namespace apsara::sls::spl