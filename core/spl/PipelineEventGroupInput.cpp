#include "spl/PipelineEventGroupInput.h"
#include <iostream>
#include "spl/util/SplStringPiece.h"
#include "logger/Logger.h"
#include "spl/SplConstants.h"


namespace apsara::sls::spl {

void PipelineEventGroupInput::getHeader(IOHeader& header, std::string& err) {
    header.rowSize = mLogGroup->GetEvents().size();
    for (auto& columnName : mColumnNames) {
        header.columnNames.emplace_back(columnName);
    }

    for (auto& kv : mLogGroup->GetTags()) {
        std::string fullTag = FIELD_PREFIX_TAG;
        fullTag += std::string(kv.first.data(), kv.first.size());
        mTmpTags.emplace_back(fullTag);
        header.columnNames.emplace_back(SplStringPiece(fullTag));
        header.constCols.emplace(header.columnNames.size() - 1, SplStringPiece(kv.second.data(), kv.second.size()));
    }
}

void PipelineEventGroupInput::getColumn(const int32_t colIndex, std::vector<SplStringPiece>& values, std::string& err) {
    std::string columnName = mColumnNames[colIndex];
    for (auto event : mLogGroup->GetEvents()) {
        LogEvent& sourceEvent = event.Cast<LogEvent>();
        StringView content = sourceEvent.GetContent(columnName);
        values.emplace_back(SplStringPiece(content.data(), content.size()));
    }
}

void PipelineEventGroupInput::getTimeColumns(std::vector<uint32_t>& times,std::vector<uint32_t>& timeNanos, std::string& err) {
    for (auto event : mLogGroup->GetEvents()) {
        LogEvent& sourceEvent = event.Cast<LogEvent>();
        times.emplace_back(sourceEvent.GetTimestamp());
        timeNanos.emplace_back(sourceEvent.GetTimestampNanosecond());
    }
}

bool PipelineEventGroupInput::isColumnar() {
    return true;
}

bool PipelineEventGroupInput::hasTime() {
    return true;
}

}  // namespace apsara::sls::spl