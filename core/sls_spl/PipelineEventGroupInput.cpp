#include "sls_spl/PipelineEventGroupInput.h"
#include <iostream>
#include "rw/SplStringPiece.h"

namespace apsara::sls::spl {

// for test

void PipelineEventGroupInput::getHeader(IOHeader& header, std::string& err) {
    header.rowSize = mLogGroup->GetEvents().size();
    for (auto& columnName : mColumnNames) {
        header.columnNames.emplace_back(columnName);
    }

    //int32_t tagSize = mLogGroup->GetGroupInfo.GetAllTags().size();
    for (auto& kv : mLogGroup->GetGroupInfo().GetAllTags()) {
        header.columnNames.emplace_back(SplStringPiece(kv.first.data(), kv.first.size()));
        header.constCols.emplace(header.columnNames.size() - 1, SplStringPiece(kv.second.data(), kv.second.size()));
    }
}
void PipelineEventGroupInput::getRow(const int32_t rowIndex, std::vector<KV>& pairs, std::string& err) {
    auto currentEvent = mLogGroup->GetEvents()[rowIndex];

    LogEvent& sourceEvent = currentEvent.Cast<LogEvent>();
    for (auto& kv : sourceEvent.GetContents()) {
        pairs.emplace_back(SplStringPiece(kv.first.data(), kv.first.size()), SplStringPiece(kv.second.data(), kv.second.size()));
    }
}

bool PipelineEventGroupInput::isColumnar() {
    return false;
}

}  // namespace apsara::sls::spl