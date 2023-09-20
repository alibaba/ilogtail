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

    std::string timestampValue = std::to_string(sourceEvent.GetTimestamp());
    mTmpSave.emplace_back(timestampValue);
    std::string timestampNanosecondValue = std::to_string(sourceEvent.GetTimestampNanosecond());
    mTmpSave.emplace_back(timestampNanosecondValue);

    pairs.emplace_back(SplStringPiece(timestamp), SplStringPiece(timestampValue));
    pairs.emplace_back(SplStringPiece(timestampNanosecond), SplStringPiece(timestampNanosecondValue));

    std::cout << rowIndex << " input timestamp:" << std::to_string(sourceEvent.GetTimestamp()) << std::endl;
    std::cout << rowIndex << " input timestampNanosecond:" << std::to_string(sourceEvent.GetTimestampNanosecond()) << std::endl;

    for (auto& kv : sourceEvent.GetContents()) {
        pairs.emplace_back(SplStringPiece(kv.first.data(), kv.first.size()), SplStringPiece(kv.second.data(), kv.second.size()));
        std::cout << rowIndex << " input content:" << SplStringPiece(kv.second.data(), kv.second.size()) << std::endl;
    }
}

bool PipelineEventGroupInput::isColumnar() {
    return false;
}

}  // namespace apsara::sls::spl