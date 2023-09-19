#pragma once

#include "rw/IO.h"

namespace apsara::sls::spl {

class PipelineEventGroupInput : public Input {
   public:
    PipelineEventGroupInput(const std::vector<std::string> columnNames, const PipelineEventGroup& rows)
        : columnNames_(columnNames), rows_(rows) {}

    ~PipelineEventGroupInput() {}

    virtual void getHeader(IOHeader& header, std::string& err);
    virtual void getRow(const int32_t rowIndex, std::vector<KV>& pairs, std::string& err);

   private:
    std::vector<std::string> columnNames_;
    std::vector<std::unordered_map<std::string, std::string>> rows_;
};

using PipelineEventGroupInputPtr = std::shared_ptr<PipelineEventGroupInput>;

}  // namespace apsara::sls::spl