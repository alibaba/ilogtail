#include "sls_spl/PipelineEventGroupInput.h"
#include <iostream>
#include "rw/SplStringPiece.h"
#include "logger/Logger.h"
#include "sls_spl/SplConstants.h"


namespace apsara::sls::spl {

void PipelineEventGroupInput::getHeader(IOHeader& header, std::string& err) {
    header.rowSize = mLogGroup->GetEvents().size();
    for (auto& columnName : mColumnNames) {
        header.columnNames.emplace_back(columnName);
    }

    for (auto& kv : mLogGroup->GetTags()) {
        header.columnNames.emplace_back(SplStringPiece(kv.first.data(), kv.first.size()));
        header.constCols.emplace(header.columnNames.size() - 1, SplStringPiece(kv.second.data(), kv.second.size()));
    }
}

void PipelineEventGroupInput::getRow(const int32_t rowIndex, std::vector<KV>& pairs, uint32_t& time, uint32_t& timeNsPart,  std::string& err) {
    auto currentEvent = mLogGroup->GetEvents()[rowIndex];
    LogEvent& sourceEvent = currentEvent.Cast<LogEvent>();

    time = sourceEvent.GetTimestamp();
    timeNsPart = sourceEvent.GetTimestampNanosecond();
    //LOG_INFO(sLogger, ("rowIndex", rowIndex)("input timestamp", std::to_string(sourceEvent.GetTimestamp())));
    //LOG_INFO(sLogger, ("rowIndex", rowIndex)("input timestampNanosecond", std::to_string(sourceEvent.GetTimestampNanosecond())));

    for (auto& kv : sourceEvent.GetContents()) {
        pairs.emplace_back(SplStringPiece(kv.first.data(), kv.first.size()), SplStringPiece(kv.second.data(), kv.second.size()));
        //LOG_INFO(sLogger, ("rowIndex", rowIndex)("input content key", SplStringPiece(kv.first.data(), kv.first.size()))("input content value", SplStringPiece(kv.second.data(), kv.second.size())));
    }
}

void PipelineEventGroupInput::getColumn(const int32_t colIndex, std::vector<SplStringPiece>& values, std::string& err) {
    
    std::string columnName = mColumnNames[colIndex];
    //LOG_INFO(sLogger, ("colIndex", colIndex)("columnName", columnName));
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
            //LOG_INFO(sLogger, ("colIndex", colIndex)("columnName", columnName)("columnValue", content));
            values.emplace_back(SplStringPiece(content.data(), content.size()));
        }
    }
}

bool PipelineEventGroupInput::isColumnar() {
    return true;
}

}  // namespace apsara::sls::spl