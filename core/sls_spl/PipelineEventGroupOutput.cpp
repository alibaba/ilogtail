#include "sls_spl/PipelineEventGroupOutput.h"
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace apsara::sls::spl {

void PipelineEventGroupOutput::setHeader(const IOHeader& header, std::string& err) {
    currentRow_ = 0;
    rowSize_ = header.rowSize;
    columnNames_ = header.columnNames;
    for (auto& constCol : header.constCols) {
        constColumns_.emplace(constCol.first, constCol.second.ToString());

        std::cout << constCol.first << constCol.second.ToString() <<std::endl;
    }
    
}

void PipelineEventGroupOutput::addRow(
    const std::vector<SplStringPiece>& row,
    const ErrorKV& errorKV,
    std::string& error) {
    std::stringstream buf;


    std::unique_ptr<LogEvent> targetEvent = LogEvent::CreateEvent(logGroups_->GetSourceBuffer());
    StringView timestamp;
    StringView timestampNanosecond;

    for (auto col = 0; col < row.size(); ++col) {
        if (row[col].hasValue()) {
            if (StringView(columnNames_[col].mPtr, columnNames_[col].mLen) == "timestamp") {
                timestamp = StringView(row[col].mPtr, row[col].mLen);
                std::cout << col << "timestamp:" << timestamp << std::endl;
            } else if (StringView(columnNames_[col].mPtr, columnNames_[col].mLen) == "timestampNanosecond") {
                timestampNanosecond = StringView(row[col].mPtr, row[col].mLen);
                std::cout << col << "timestampNanosecond:" << timestampNanosecond << std::endl;
            } else {
                targetEvent->SetContentNoCopy(StringView(columnNames_[col].mPtr, columnNames_[col].mLen), StringView(row[col].mPtr, row[col].mLen));
                std::cout << col << "content:" << StringView(row[col].mPtr, row[col].mLen) << std::endl;
            }
        }
    }

    targetEvent->SetTimestamp(atol(timestamp.data()), atol(timestampNanosecond.data()));

    newEvents_->emplace_back(std::move(targetEvent));
    if (!errorKV.second.empty()) {
        buf << std::setw(36) << ("__error__") << ": " << errorKV.second << std::endl;
    }
    if (currentRow_ == rowSize_ - 1) {
        std::cout << buf.str();
    } else {
        std::cout << buf.str() << std::endl;
    }
    ++currentRow_;
    if (withSleep_) {
        sleep(1);
    }
}


void PipelineEventGroupOutput::finish(std::string& error) {
    std::cout << "#######====================================================#######" << std::endl;
    std::cout << "Results: " << rowSize_ << " Rows.\n"
              << std::endl;
}

}  // namespace apsara::sls::spl