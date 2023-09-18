#pragma once

#include "rw/IO.h"

namespace apsara::sls::spl {

class MapRowInput : public Input {
   public:
    MapRowInput(const std::vector<std::string> columnNames, const std::vector<std::unordered_map<std::string, std::string>>& rows)
        : columnNames_(columnNames), rows_(rows) {}

    ~MapRowInput() {}

    virtual void getHeader(IOHeader& header, std::string& err);
    virtual void getRow(const int32_t rowIndex, std::vector<KV>& pairs, std::string& err);

   private:
    std::vector<std::string> columnNames_;
    std::vector<std::unordered_map<std::string, std::string>> rows_;
};

using MapRowInputPtr = std::shared_ptr<MapRowInput>;

}  // namespace apsara::sls::spl