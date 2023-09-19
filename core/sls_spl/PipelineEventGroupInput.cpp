#include "sls_spl/PipelineEventGroupInput.h"
#include <iostream>
#include "rw/SplStringPiece.h"

namespace apsara::sls::spl {

// for test

void PipelineEventGroupInput::getHeader(IOHeader& header, std::string& err) {
    header.rowSize = rows_->GetEvents().size();
    for (auto& columnName : columnNames_) {
        header.columnNames.emplace_back(columnName);
    }
}
void PipelineEventGroupInput::getRow(const int32_t rowIndex, std::vector<KV>& pairs, std::string& err) {
    auto currentRow = rows_->GetEvents()[rowIndex];

    LogEvent& sourceEvent = currentRow.Cast<LogEvent>();
    for (auto& kv : sourceEvent.GetContents()) {
        pairs.emplace_back(SplStringPiece(kv.first.data(), kv.first.size()), SplStringPiece(kv.second.data(), kv.second.size()));
    }
}

}  // namespace apsara::sls::spl