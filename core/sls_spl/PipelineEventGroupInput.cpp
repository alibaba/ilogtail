#include "sls_spl/PipelineEventGroupInput.h"
#include <iostream>
#include "spl/util/SplStringPiece.h"
#include "logger/Logger.h"
#include "sls_spl/SplConstants.h"


namespace apsara::sls::spl {

void PipelineEventGroupInput::getHeader(IOHeader& header, std::string& err) {
    header.rowSize = mLogGroup->GetEvents().size();
    for (auto& columnName : mColumnNames) {
        header.columnNames.emplace_back(columnName);
    }

    for (auto& kv : mLogGroup->GetTags()) {
        std::string fullTag = FIELD_PREFIX_TAG;
        fullTag += kv.first;
        mTmpTags.emplace_back(fullTag);
        header.columnNames.emplace_back(SplStringPiece(fullTag));
        header.constCols.emplace(header.columnNames.size() - 1, SplStringPiece(kv.second.data(), kv.second.size()));
    }
}

void PipelineEventGroupInput::getColumn(const int32_t colIndex, std::vector<SplStringPiece>& values, std::string& err) {
    std::string columnName = mColumnNames[colIndex];
    LOG_DEBUG(sLogger, ("colIndex", colIndex)("columnName", columnName));
    for (auto event : mLogGroup->GetEvents()) {
        LogEvent& sourceEvent = event.Cast<LogEvent>();
        if (FIELD_TIMESTAMP == columnName) {
            std::string timestampValue = std::to_string(sourceEvent.GetTimestamp());
            values.emplace_back(SplStringPiece(timestampValue));
        } else if (FIELD_TIMESTAMP_NANOSECOND == columnName) { 
            std::string timestampNanosecondValue = std::to_string(sourceEvent.GetTimestampNanosecond());
            values.emplace_back(SplStringPiece(timestampNanosecondValue));
        } else {
            StringView content = sourceEvent.GetContent(columnName);
            values.emplace_back(SplStringPiece(content.data(), content.size()));
        }
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