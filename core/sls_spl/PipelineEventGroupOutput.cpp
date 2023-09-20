#include "sls_spl/PipelineEventGroupOutput.h"
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "logger/Logger.h"


namespace apsara::sls::spl {

void PipelineEventGroupOutput::setHeader(const IOHeader& header, std::string& err) {
    mRowSize = header.rowSize;
    mColumnNames = header.columnNames;
    for (auto& columeName : mColumnNames) {
        LOG_INFO(sLogger, ("columeName", columeName));
    }
    for (auto& constCol : header.constCols) {
        mConstColumns.emplace(constCol.first, constCol.second.ToString());
        LOG_INFO(sLogger, ("constCol key", constCol.first)("constCol value", constCol.second.ToString()));
    }
}

void PipelineEventGroupOutput::addRow(
    const std::vector<SplStringPiece>& row,
    const ErrorKV& errorKV,
    std::string& error) {

    std::unique_ptr<LogEvent> targetEvent = LogEvent::CreateEvent(mLogGroup->GetSourceBuffer());
    StringView timestamp;
    StringView timestampNanosecond;

    LOG_INFO(sLogger, ("row.size()", row.size()));
    for (auto col = 0; col < row.size(); ++col) {
        if (row[col].hasValue()) {
            if (StringView(mColumnNames[col].mPtr, mColumnNames[col].mLen) == "timestamp") {
                timestamp = StringView(row[col].mPtr, row[col].mLen);
            } else if (StringView(mColumnNames[col].mPtr, mColumnNames[col].mLen) == "timestampNanosecond") {
                timestampNanosecond = StringView(row[col].mPtr, row[col].mLen);
            } else {
                targetEvent->SetContent(StringView(mColumnNames[col].mPtr, mColumnNames[col].mLen), StringView(row[col].mPtr, row[col].mLen));

                LOG_INFO(sLogger, ("content key", StringView(mColumnNames[col].mPtr, mColumnNames[col].mLen))("content value", StringView(row[col].mPtr, row[col].mLen)));
            }
        }
    }

    targetEvent->SetTimestamp(atol(timestamp.data()), atol(timestampNanosecond.data()));
    mNewEvents->emplace_back(std::move(targetEvent));
    if (!errorKV.second.empty()) {
        LOG_INFO(sLogger, ("__error__", errorKV.second));
    }
    
    if (mWithSleep) {
        sleep(1);
    }
}


void PipelineEventGroupOutput::finish(std::string& error) {

}

}  // namespace apsara::sls::spl